/*
 *  Copyright (C) 2004 Steve Harris, Uwe Koloska
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#include <alsa/asoundlib.h>
#include "lo/lo.h"
#include "dsp_communication.h"
#include "adi_xml_parse.h"
#include "adi_osc.h"


/* ARM Control */
int done = 0;
static char *card = "hw:0";
static char *test_wav = "/test_8000_32bit.wav";
static snd_ctl_t *snd_handle = NULL;
static snd_ctl_elem_list_t *list;
static snd_ctl_elem_info_t *info;
static snd_ctl_elem_value_t *ctl_elem_value;
static struct adi_osc *arm_widget;
#define DEFAULT_ARM_XML_FILE "/arm_widget.xml"

/* DSP Control */
static int block_mode = 1;
static int connection_id = 3;
/* fd to operation DSP communication */
static int dsp_fd; 
static struct adi_osc *dsp_widget;
#define DEFAULT_DSP_XML_FILE "/dsp_widget.xml"

static int start_osc_server(int osc_port, int num1, int num2, lo_server_thread osc_server) {
	int i;
	uint32_t port = (osc_port > 100 && osc_port < 60000) ? osc_port : 9988;
	char sport[10];

	sprintf(sport,"%d", port);

	osc_server = lo_server_thread_new(sport, error);
	if (!osc_server) {
		fprintf(stderr, "OSC failed to listen on port %s", sport);
		return -1;
	}

	/* Register arm_handler */
	for (i = 0;i < num1;++i)
	{
		lo_server_thread_add_method(osc_server, arm_widget[i].path,
					arm_widget[i].type, arm_handler, &num1);
	}

	/* Register dsp_handler */
	for (i = 0;i < num2;++i)
	{
		lo_server_thread_add_method(osc_server, dsp_widget[i].path,
					dsp_widget[i].type, dsp_handler, &num2);
	}

	lo_server_thread_start(osc_server);

	return 0;
}

static void stop_osc_server(lo_server_thread osc_server) {
	if (!osc_server)
	  return;
	lo_server_thread_stop(osc_server);
	lo_server_thread_free(osc_server);
	fprintf(stderr, "stop osc server\n");
	osc_server = NULL;
}

static void amixer_control_ops(int id, lo_arg *argv)
{
	char *name;
    int i,used,amixer_numid,err = 0;
	unsigned int idx, count;

	/* find the amixer numid acording to the amixer name */
	used = snd_ctl_elem_list_get_used(list);

	for (i=0; i < used; i++) {
		name = snd_ctl_elem_list_get_name(list, i);
		if(strcmp(name, arm_widget[id].desc) == 0)
		{
			amixer_numid = snd_ctl_elem_list_get_numid(list, i);
			break;
		}
	}

	snd_ctl_elem_info_set_numid(info, amixer_numid);
	snd_ctl_elem_info(snd_handle, info);
	count = snd_ctl_elem_info_get_count(info);

	snd_ctl_elem_value_set_numid(ctl_elem_value, amixer_numid); 
	for (idx = 0; idx < count; idx++)
		snd_ctl_elem_value_set_integer(ctl_elem_value, idx, argv->i);
       
	if ((err = snd_ctl_elem_write(snd_handle, ctl_elem_value)) < 0) {
		printf("Control %s element write error: %s", card, snd_strerror(err));
	}
}
static void aplay_control_ops(lo_arg *argv)
{
	char play_cmd[40];
	sprintf(play_cmd, "aplay -D adi_adsp %s&",test_wav); 

	if (*(int32_t *)argv)
		system(play_cmd);
	else
		system("kill -9 $(ps -ef| grep aplay| grep -v grep | awk '{print $1}')");
}

static void signal_handler(int sig)
{
	done = 1;
	signal(sig, signal_handler);
}

static int help(void)
{
	printf("Usage: osc_example <options>\n");
	printf("\nAvailable options:\n");
	printf("\t-h,--help\t\tthis help\n");
	printf("\t-f,--file\t\tload xml parameter file\n");
	printf("\t-p,--port\t\tosc server listen port\n");
	return 0;
}

int main(int argc, char *argv[])
{
    int err = 0;
	int ret = 0;
	int i, osc_port, sram_size = 0;
	static const char short_options[] = "hf:p:";
	lo_server_thread st = NULL;
	char *dsp_xml_file = DEFAULT_DSP_XML_FILE;
	char *arm_xml_file = DEFAULT_ARM_XML_FILE;
	int dsp_widget_num = 0;
	int arm_widget_num = 0;

	static const struct option long_options[] = {
		{ "help", 0, NULL, 'h' },
		{ "xml", 1, NULL, 'f' },
		{ "port", 1, NULL, 'p' },
	};

	/* Parse parameter */
	while (1) {
		int c;
		if ((c = getopt_long(argc, argv, short_options, long_options, NULL)) < 0)
			break;
		switch (c) {
		case 'h':
			help();
			return 0;
		case 'p':
			osc_port = atoi(optarg);
			break;
		case 'f':
			dsp_xml_file = optarg;
			printf("TODO!!!!\n");
			break;
		default:
			help();
			break;
		}
	}

	/* Prepare for ARM control ops */
    if (snd_handle == NULL && (ret = snd_ctl_open(&snd_handle, card, 0)) < 0) {
        printf("Control %s open error: %s", card, snd_strerror(ret));
        return ret;
    }

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_value_alloca(&ctl_elem_value);

	snd_ctl_elem_list_malloc(&list);
	snd_ctl_elem_list_alloc_space(list, 100);
	snd_ctl_elem_list(snd_handle, list);

	if((ret = snd_ctl_elem_value_malloc (&ctl_elem_value)) < 0){
        printf("Control %s open error: %s", card, snd_strerror(ret));
		goto free_snd;
    }

	arm_widget_num = get_xml_node_count(arm_xml_file, "arm_parameter");
	arm_widget = malloc(arm_widget_num * sizeof(struct adi_osc));
	if (!arm_widget) {
	  fprintf(stderr, "Failed to malloc arm widget memory\n");
	  ret = -1;
	  goto free_snd;
	}

	xml_parse(arm_xml_file, "arm_parameter", arm_widget);

	/*  Prepare for DSP control ops */
	dsp_widget_num = get_xml_node_count(dsp_xml_file, "dsp_parameter"); 
	dsp_widget = malloc(dsp_widget_num * sizeof(struct adi_osc));
	if (!dsp_widget) {
	  fprintf(stderr, "Failed to malloc dsp widget memory\n");
	  ret = -1;
	  goto free_arm_widget;
	}

	xml_parse(dsp_xml_file, "dsp_parameter", dsp_widget);
	
	sram_size = dsp_widget[dsp_widget_num -1].base + dsp_widget[dsp_widget_num -1].width; 

	dsp_fd = dsp_com_open(connection_id, sram_size, block_mode);
	if (!dsp_fd) {
		fprintf(stderr, "Failed to open dsp communication\n");
		ret = -1;
		goto free_dsp_widget;
	}

	/*  Start OSC server */
	ret = start_osc_server(osc_port, arm_widget_num, dsp_widget_num, st);
	if (ret < 0) {
		fprintf(stderr, "Error in starting osc server\n");
		goto free_dsp_com;
	}

//	signal(SIGINT, signal_handler);
//	signal(SIGTERM, signal_handler);
//	signal(SIGABRT, signal_handler);

    while (!done) {
#ifdef WIN32
    Sleep(1);
#else
	usleep(1000);
#endif
	}


free_osc_server:
	stop_osc_server(st);
free_dsp_com:
	dsp_com_close(dsp_fd);
free_dsp_widget:
	free(dsp_widget);
free_arm_widget:
	free(arm_widget);
free_snd:
    snd_ctl_close(snd_handle);
    snd_handle = NULL;

   return ret;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int arm_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
	int i, id;
	lo_arg *msg = malloc(sizeof(lo_arg));
	int arm_widget_num = *(int32_t *)user_data;

	for (i = 0; i < argc; i++) {
		for(id = 0; id < arm_widget_num; id++) {
			if(strcmp(arm_widget[id].path, path) == 0)
			  break;
		}
		/****************** special aplay widget *************************/
	
		if(strcmp(arm_widget[id].desc, "aplay command") == 0)
			aplay_control_ops(argv[i]);
		else {
			/******************* lineout volume  ***********************
			 * first change the value from dB to interger */
			if(strcmp(arm_widget[id].desc, AMIXER_LINEOUT_PLAYBACK_VOLUME) == 0)
				vol_db_to_int(msg, argv[i]->i, 63, 0, 6, -57);
			else
				msg = argv[i];
			amixer_control_ops(id, msg);
		}
	}
}

int dsp_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
	int i,id;
    lo_pcast32 val32;
    lo_pcast64 val64;
	int bigendian = 0;
    int size;
	int dsp_widget_num = *(int32_t *)user_data;

	for(id = 0; id < dsp_widget_num; id++) {
		if(strcmp(dsp_widget[id].path, path) == 0)
		  break;
	}

	/*  If one address has two args */
	for (i = 0; i < argc; i++)
	{
		/* process path value */
	    size = lo_arg_size(types[i], argv[i]);
		if (size == 4 || types[i] == LO_BLOB) {
			if (bigendian) {
				val32.nl = lo_otoh32(*(int32_t *)argv[i]);
			} else {
			    val32.nl = *(int32_t *)argv[i];
			}
	    } else if (size == 8) {
			if (bigendian) {
				val64.nl = lo_otoh64(*(int64_t *)argv[i]);
			} else {
				val64.nl = *(int64_t *)argv[i];
			}
		}
		
		if (size > dsp_widget[id].width) {
			fprintf(stderr, "widget[%d],value size:%d, out of range %d!\n",
					id,	size, dsp_widget[id].width);
			size = dsp_widget[id].width;
		}

		dsp_com_write(dsp_fd, &val32, size, dsp_widget[id].base); 
	}
}

static int vol_db_to_int(lo_arg *result, int argv, 
			int dest_max, int dest_min, 
			int db_max, int db_min){
	int i;

	int dest_delta = dest_max - dest_min;
	int db_delta = db_max - db_min;
	int dest_to_db_delta = argv - db_min;

	result->i = (dest_delta/db_delta)*dest_to_db_delta + dest_min;
	return 1;
}
