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

	printf("hfeng %s %d, sport=%s\n",__func__,__LINE__,sport);
	osc_server = lo_server_thread_new(sport, error);
	if (!osc_server) {
		fprintf(stderr, "OSC failed to listen on port %s", sport);
		return -1;
	}

	/* Register arm_handler */
	for (i = 0;i < num1;++i)
	{
		printf("hfeng %s\t:Register path=%s, type=%s\n",__func__,arm_widget[i].path,arm_widget[i].type);
		lo_server_thread_add_method(osc_server, arm_widget[i].path,
					arm_widget[i].type, arm_handler, &num1);
	}

	/* Register dsp_handler */
	for (i = 0;i < num2;++i)
	{
		printf("hfeng %s\t:Register path=%s, type=%s\n",__func__,dsp_widget[i].path,dsp_widget[i].type);
		lo_server_thread_add_method(osc_server, dsp_widget[i].path,
					dsp_widget[i].type, dsp_handler, &num2);
	}

	lo_server_thread_start(osc_server);

	return 0;
}

static void stop_osc_server(lo_server_thread osc_server) {
	printf("hfeng %s %d",__func__,__LINE__);
	if (!osc_server)
	  return;
	lo_server_thread_stop(osc_server);
	lo_server_thread_free(osc_server);
	fprintf(stderr, "stop osc server\n");
	osc_server = NULL;
}

#if 0
static void show_control_id(snd_ctl_elem_id_t *id)
{
	char *str;

	str = snd_ctl_ascii_elem_id_get(id);
	if (str)
		printf("%s", str);
	free(str);
}


static void arm_control_ops(const char *path, lo_type type,
			lo_arg *argv, void *user_data)
{
	printf("hfeng %s %d path=%s\n",__func__,__LINE__, path);
	char *name;
    int i,used,amixer_numid,err = 0;
	unsigned int idx, count;
	lo_arg *msg = malloc(sizeof(lo_arg));
	int ops_id = *(int32_t *)user_data;
	char aplay_cmd[40];

	if(strcmp(user_wid[ops_id].desc, "aplay") == 0){
		sprintf(aplay_cmd, "aplay -D %s,0 %s&",card,test_wav); 

		if (*(int32_t *)argv)
			system(aplay_cmd);
		else
			system("kill -9 $(ps -ef| grep aplay| grep -v grep | awk '{print $1}')");
		return;
	}

	/* if the it is the lineout volume ops
	 * first change the value from dB to interger */
	if(strcmp(user_wid[ops_id].desc, AMIXER_LINEOUT_PLAYBACK_VOLUME) == 0)
		vol_db_to_int(msg, argv->i, 63, 0, 6, -57);
	else
		msg = argv;

//	printf("hfeng %d path=%s\n",__LINE__, path);
	/* find the amixer numid acording to the amixer name */
	used = snd_ctl_elem_list_get_used(list);

	for (i=0; i < used; i++) {
		name = snd_ctl_elem_list_get_name(list, i);
		if(strcmp(name, user_wid[ops_id].desc) == 0)
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
		snd_ctl_elem_value_set_integer(ctl_elem_value, idx, msg->i);
       
	if ((err = snd_ctl_elem_write(snd_handle, ctl_elem_value)) < 0) {
		printf("Control %s element write error: %s", card, snd_strerror(err));
	}
}
#endif
static void amixer_control_ops(int id, lo_arg *argv)
{
	char *name;
    int i,used,amixer_numid,err = 0;
	unsigned int idx, count;
	printf("hfeng %s %d \n",__func__,__LINE__);
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
	printf("hfeng %s %d\t \n",__func__,__LINE__);
	char play_cmd[40];
	sprintf(play_cmd, "aplay -D adi_adsp %s&",test_wav); 

	if (*(int32_t *)argv)
	{
		system(play_cmd);
		printf("hfeng %s %d\t %s\n",__func__,__LINE__,play_cmd);
	}
	else {
		printf("hfeng %s %d kill aplay\n",__func__,__LINE__);
		system("kill -9 $(ps -ef| grep aplay| grep -v grep | awk '{print $1}')");
	}
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

	printf("hfeng %s %d",__func__,__LINE__);
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

	printf("hfeng end osc_example program \n");

free_osc_server:
	printf("hfeng %s %d",__func__,__LINE__);
	stop_osc_server(st);
free_dsp_com:
	printf("hfeng %s %d",__func__,__LINE__);
	dsp_com_close(dsp_fd);
free_dsp_widget:
	printf("hfeng %s %d",__func__,__LINE__);
	free(dsp_widget);
free_arm_widget:
	printf("hfeng %s %d",__func__,__LINE__);
	free(arm_widget);
free_snd:
	printf("hfeng %s %d",__func__,__LINE__);
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
	printf("hfeng %s %d path=%s\n",__func__,__LINE__, path);
	int i, id;
	lo_arg *msg = malloc(sizeof(lo_arg));
	int arm_widget_num = *(int32_t *)user_data;

	for (i = 0; i < argc; i++) {
		printf("hfeng %s %d, arm_widget_num=%d\n",__func__,__LINE__,arm_widget_num);

		for(id = 0; id < arm_widget_num; id++) {
			if(strcmp(arm_widget[id].path, path) == 0)
			  break;
		}
		/****************** special aplay widget *************************/
	
		printf("hfeng %s %d\t id=%d, argv=%d\n",__func__,__LINE__,id,argv[i]->i);
		if(strcmp(arm_widget[id].desc, "aplay command") == 0)
			aplay_control_ops(argv[i]);
		else {
	
			printf("hfeng %s %d path=%s\n",__func__,__LINE__, path);
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

	printf("hfeng %s %d, dsp_widget_num=%d\n",__func__,__LINE__,dsp_widget_num);
	for(id = 0; id < dsp_widget_num; id++) {
		if(strcmp(dsp_widget[id].path, path) == 0)
		  break;
	}

    printf("hfeng %s %d path: <%s>,id=%d,argc=%d,adi_path=%s, adi_desc=%s\n",__func__,__LINE__, path,id,argc,dsp_widget[id].path,dsp_widget[id].desc);

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
		printf("hfeng %s %d dsp_control_ops finished\n",__func__,__LINE__);
	}
}
#if 0
int user_widget_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
	int i,id;
    printf("path: <%s>\n", path);

	for(id = 0; id < sizeof(user_wid); id++) {
		if(strcmp(user_wid[id].osc_path, path) == 0)
		  break;
	}

	/* Do the corresponding ops */
	for (i=0; i<argc; i++) {
		printf("user_widget_handler arg %d '%c'\n ", i, types[i]);
		lo_arg_pp(types[i], argv[i]);

		switch(user_wid[id].ctr_type)
		{
		case ARM:
			arm_control_ops(path, types[i], argv[i], &id);
			break;
		case DSP:
			dsp_control_ops(path, types[i], argv[i], &id);
			printf("hfeng %s %d dsp_control_ops finished\n",__func__,__LINE__);
			break;
		case ECHO_SWITCH:
			echo_loopback_control_ops(path, types[i], argv[i], &id);
			break;
		case POT_HADC_SWITCH:
			pot_hadc_loopback_control_ops(path, types[i], argv[i], &id);
			break;
//		case APLAY_SWITCH:
//			musicplay_control_ops(path, types[i], argv[i], &id);
		}
	}
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");
    fflush(stdout);

    return 0;
}
#endif
int vol_db_to_int(lo_arg *result, int argv, 
			int dest_max, int dest_min, 
			int db_max, int db_min){
//	printf("hfeng1 argv=%d\n", argv);
	int i;

	int dest_delta = dest_max - dest_min;
	int db_delta = db_max - db_min;
	int dest_to_db_delta = argv - db_min;

//	printf("hfeng1 dest_delta=%d\n", dest_delta);
//	printf("hfeng1 db_delta=%d\n", db_delta);
//	printf("hfeng1 dest_to_db_delta=%d\n", dest_to_db_delta);

	result->i = (dest_delta/db_delta)*dest_to_db_delta + dest_min;
//	printf("hfeng1 result=%d\n", result->i);
	return 1;
}
