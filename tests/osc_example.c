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
#include "dsp_communication.h"

#include <libxml/parser.h>  
#include <libxml/xmlmemory.h>

#include "lo/lo.h"
#include "adi_osc.h"

/* DSP Control */
//static COM master = {0};
//static COM slave = {0};
static volatile EFFECT_PARAM effect_param = {0};
int mode = 0;
static int block_mode = 1; // block = 1, noblock = 0
int timeout = 5*1000;
//static DSP_COM_HANDLE dsp_handle = {0};
static int dsp_fd; // fd to operation DSP communication
static int connection_id = 3;
#define BUFF_SIZE 64
#define MAX_BUF_SIZE BUFF_SIZE*1024
#define MAX_WIDGET_NUM 1000


/* ARM Control */
int done = 0;
static char *card = "hw:0";
static char *test_wav = "/test-48k.wav";
static snd_ctl_t *handle = NULL;
static snd_ctl_elem_list_t *list;
static snd_ctl_elem_info_t *info;
static snd_ctl_elem_value_t *ctl_elem_value;

static struct widget user_wid[] = {
#if 1 
	{ARM, LO_INT32, "/aplay/switch", "aplay"},
	{ARM, LO_INT32, "/lineout/switch", AMIXER_LINEOUT_PLAYBACK_SWITCH},
	{ARM, LO_INT32, "/lineout/volume", AMIXER_LINEOUT_PLAYBACK_VOLUME},
	{ARM, LO_INT32, "/left/LR/left/volume", AMIXER_LEFT_LR_PLAYBACK_MIXER_LEFT_VOLUME},
	{ARM, LO_INT32, "/left/LR/right/volume", AMIXER_LEFT_LR_PLAYBACK_MIXER_RIGHT_VOLUME},
	{ARM, LO_INT32, "/right/LR/left/volume", AMIXER_RIGHT_LR_PLAYBACK_MIXER_LEFT_VOLUME},
	{ARM, LO_INT32, "/right/LR/right/volume", AMIXER_RIGHT_LR_PLAYBACK_MIXER_RIGHT_VOLUME},
	{ARM, LO_INT32, "/DAC/playback/mux", AMIXER_DAC_PLAYBACK_MUX},
	{ARM, LO_INT32, "/left/left/DAC/switch", AMIXER_LEFT_PLAYBACK_MIXER_LEFT_DAC_SWITCH},
	{ARM, LO_INT32, "/left/right/DAC/switch", AMIXER_LEFT_PLAYBACK_MIXER_RIGHT_DAC_SWITCH},
	{ARM, LO_INT32, "/right/left/DAC/switch", AMIXER_RIGHT_PLAYBACK_MIXER_LEFT_DAC_SWITCH},
	{ARM, LO_INT32, "/right/right/DAC/switch", AMIXER_RIGHT_PLAYBACK_MIXER_RIGHT_DAC_SWITCH},
#if 0
	{DSP, LO_FLOAT, "/audioproj/fin/pot/hadc0", NULL},
	{DSP, LO_FLOAT, "/audioproj/fin/pot/hadc1", NULL},
	{DSP, LO_FLOAT, "/audioproj/fin/pot/hadc2", NULL},
	{DSP, LO_INT32, "/effects/preset", NULL},
	{DSP, LO_INT32, "/reverb/preset", NULL},
	{DSP, LO_FLOAT, "/dsp/gain/delay/seconds", NULL},
	{DSP, LO_FLOAT, "/dsp/gain/feedback", NULL},
	{DSP, LO_FLOAT, "/dsp/gain/wet/mix", NULL},
	{DSP, LO_INT32, "/echo/loopback/switch", "echo loopback switch"}, 
	{DSP, LO_INT32, "/pot/hadc/loopback/switch", "pot hadc loopback switch"}, 
#endif
#endif
};
static struct osc_ops amixer_audio_ops[] = {
};
#if
static struct adi_dsp_osc dsp_osc_widget[] = {
	{0, "/audioproj/fin/pot/hadc0", "f", "set hadc0 parameter"},
	{1, "/audioproj/fin/pot/hadc1", "f", "set hadc1 parameter"},
	{2, "/audioproj/fin/pot/hadc2", "f", "set hadc2 parameter"},
	{3, "/audioproj/fin/pot/hadc3", "f", "set hadc3 parameter"},
	{4, "/audioproj/fin/pot/hadc4", "f", "set hadc4 parameter"},
	{5, "/audioproj/fin/pot/hadc5", "f", "set hadc5 parameter"},
	{6, "/audioproj/fin/pot/hadc6", "f", "set hadc6 parameter"},
	{7, "/effects/preset", "i", "enable effects preset"},
	{8, "/reverb/preset", "i", "enable reverb preset"},
	{9, "/total/effects/preset", "i", "enable total effexts preset"},
#if 0
	{5, "/dsp/gain/delay/seconds", "f", "set gain delay seconds"},
	{6, "/dsp/gain/feedback", "f", "set gain feedback"},
	{7, "/dsp/gain/wet/mix", "f", "set gain wet mix value"},
	{8, "/echo/loopback/switch", "i", "echo loopback switch"}, 
	{9, "/pot/hadc/loopback/switch", "i", "pot hadc loopback switch"}, 
#endif
};

static int start_osc_server(int osc_port, lo_server_thread osc_server) {
	int i;
	uint32_t port = (osc_port > 100 && osc_port < 60000) ? osc_port : 9988;
	char sport[10];

	sprintf(sport,"%d", port);

	printf("hfeng %s %d, sport=%s\n",__func__,__LINE__,sport);
//	itoa(port, sport, 10);
	osc_server = lo_server_thread_new(sport, error);

	if (!osc_server) {
		fprintf(stderr, "OSC failed to listen on port %s", sport);
		return -1;
	}

	/* Register dsp_handler */
	printf("hfeng %s %d\n",__func__,__LINE__);
	for (i = 0;i < sizeof(dsp_osc_widget)/sizeof(struct adi_dsp_osc);++i)
	{
		lo_server_thread_add_method(osc_server, dsp_osc_widget[i].path,
					dsp_osc_widget[i].type, dsp_handler, NULL);
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
	snd_ctl_elem_info(handle, info);
	count = snd_ctl_elem_info_get_count(info);

	snd_ctl_elem_value_set_numid(ctl_elem_value, amixer_numid); 
	for (idx = 0; idx < count; idx++)
		snd_ctl_elem_value_set_integer(ctl_elem_value, idx, msg->i);
       
	if ((err = snd_ctl_elem_write(handle, ctl_elem_value)) < 0) {
		printf("Control %s element write error: %s", card, snd_strerror(err));
	}
}

static void musicplay_control_ops(const char *path, lo_type type,
			lo_arg *argv, void *user_data)
{
//	printf("hfeng %d path=%s\n",__LINE__, path);
	char play_cmd[40];
	sprintf(play_cmd, "aplay -D %s,0 %s&",card,test_wav); 

	if (*(int32_t *)argv)
		system(play_cmd);
	else
		system("kill -9 $(ps -ef| grep aplay| grep -v grep | awk '{print $1}')");
}


static void dsp_control_ops(const char *path, lo_type type,
			lo_arg *argv, void *user_data)
{
	int id = *(int32_t *)user_data;
	printf("\nhfeng%s %d path=%s\n",__func__,__LINE__, path);
	printf("%s fdfdfd =0x%x\n",__func__,dsp_fd);

}

static void pot_hadc_loopback_control_ops(const char *path, lo_type type,
			lo_arg *argv, void *user_data)
{
	printf("hfeng%s %d path=%s\n",__func__,__LINE__, path);
	if (*(int32_t *)argv)
	{
		system("sh /pot_hadc_startup.sh");
	}
	else {
		system("sh /pot_hadc_stop.sh");
		//system("kill -9 $(ps -ef| grep aplay| grep -v grep | awk '{print $1}')");
		//system("kill -9 $(ps -ef| grep arecord| grep -v grep | awk '{print $1}')");
	}
}

static void echo_loopback_control_ops(const char *path, lo_type type,
			lo_arg *argv, void *user_data)
{
	printf("hfeng %s %d path=%s\n",__func__,__LINE__, path);
	if (*(int32_t *)argv)
	{
		system("sh /echo_startup.sh");
	}
	else {
		system("sh /echo_stop.sh");
//		system("kill -9 $(ps -ef| grep aplay| grep -v grep | awk '{print $1}')");
//		system("kill -9 $(ps -ef| grep arecord| grep -v grep | awk '{print $1}')");

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

static int offset[sizeof(dsp_osc_widget)/sizeof(struct adi_dsp_osc)];
int main(int argc, char *argv[])
{
    int err = 0;
	int i, osc_port, offset_size = 0;
	static const char short_options[] = "hf:p:";
	lo_server_thread st = NULL;

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
			break;
		case 'p':
			osc_port = atoi(optarg);
			break;
		case 'f':
			printf("TODO!!!!\n");
		default:
			help();
			break;
		}
	}

	offset[0] = 0;
	/* Calculate each offset */
	for (i = 1; i < sizeof(dsp_osc_widget)/sizeof(struct adi_dsp_osc); i++)
	{
		if (dsp_osc_widget[i].type == "i")
			offset_size = sizeof(int32_t);
		else if (dsp_osc_widget[i].type == "f")
			offset_size = sizeof(float);

		offset[i] = offset[i-1] + offset_size;
		printf("hfeng offset[%d]=%d\n",i, offset[i]);
	}

//	state_parse_handler();

	/* Prepare for ARM control ops */
    if (handle == NULL && (err = snd_ctl_open(&handle, card, 0)) < 0) {
        printf("Control %s open error: %s", card, snd_strerror(err));
        return err;
    }

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_value_alloca(&ctl_elem_value);

	snd_ctl_elem_list_malloc(&list);
	snd_ctl_elem_list_alloc_space(list, 100);
	snd_ctl_elem_list(handle, list);

	if((err = snd_ctl_elem_value_malloc (&ctl_elem_value)) < 0){
        printf("Control %s open error: %s", card, snd_strerror(err));
        snd_ctl_close(handle);
        handle = NULL;
        return err;
    }

	/*  Prepare for DSP control ops */
	dsp_fd = dsp_com_open(connection_id, offset_size, block_mode);

	/*  Start OSC server */
	start_osc_server(osc_port, st);

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);

    while (!done) {
#ifdef WIN32
    Sleep(1);
#else
	usleep(1000);
#endif
	}

	stop_osc_server(st);
	dsp_com_close(dsp_fd);

    return 0;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
    int i,id;

    printf("path: <%s>\n", path);

	/* To fine the id in the ops acording to the path name */
	for (id=0;id < sizeof(amixer_audio_ops);id++) {
		if(strcmp(amixer_audio_ops[id].osc_path, path) == 0)
			break;
	}
//	printf("hfeng id=%d, path=%s, osc_path=%s\n",id,path,amixer_audio_ops[id].osc_path);
	/* Do the corresponding ops */
	for (i=0; i<argc; i++) {
		printf("arg %d '%c' ", i, types[i]);
		lo_arg_pp(types[i], argv[i]);
		amixer_audio_ops[id].do_audio_control(path, &types[i], argv[i], &id);
		printf("\n");
	}
    printf("\n");

    fflush(stdout);
 
    return 1;
}

int dsp_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
	int i,id;
    lo_pcast32 val32;
    lo_pcast64 val64;
	int bigendian = 0;
    int size;

	for(id = 0; id < sizeof(dsp_osc_widget); id++) {
		if(strcmp(dsp_osc_widget[id].path, path) == 0)
		  break;
	}

    printf("hfeng %s %d path: <%s>,id=%d,argc=%d,adi_path=%s, adi_desc=%s\n",__func__,__LINE__, path,id,argc,dsp_osc_widget[id].path,dsp_osc_widget[id].desc);

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

		dsp_com_write(dsp_fd, &val32, size, offset[id]); 
		printf("hfeng %s %d dsp_control_ops finished\n",__func__,__LINE__);
	}
}
int user_widget_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
	int i,id;
    printf("path: <%s>\n", path);

	for(id = 0; id < sizeof(user_wid); id++) {
		if(strcmp(user_wid[id].osc_path, path) == 0)
		  break;
	}
//	printf("hfeng 422 id=%d, path=%s, user_wid[id]=%s\n", id, path, user_wid[id].osc_path);
//	printf("hfeng user_wid[%d].addr=%f\n", id, *user_wid[id].addr);

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
void state_parse_handler()	    
{
	FILE *fp;
	int id;
	char txt[101];

	if((fp = fopen("/sam_audio.state","r")) == NULL)
	{
		printf("No sam_audio.state file in root directory\n\
				Please copy your sam_audio.state file into filesystem\n");
		exit(1);
	}
	
	while(!feof(fp))
	{
		fgets(txt, 100, fp);
		delSpace(txt);

		if ((strstr(txt,"[") == NULL) && (strstr(txt,"]") == NULL))
		{
			for (id = 0; id < (sizeof(amixer_audio_ops)/sizeof(amixer_audio_ops[0])); id++){
				if(strcmp(amixer_audio_ops[id].osc_path, txt) == 0){
					memcpy(amixer_audio_ops[id].osc_id, txt, strlen(txt));

					fgets(txt, 100, fp);
					delSpace(txt);
					amixer_audio_ops[id].osc_value = atof(txt);
				}
			}
		}
	}

	fclose(fp);
}

void delSpace(char *str)
{
	char *p = str;
	char *t = p;

	while (*p != '\0') {
		if (*p == ' ' ) {
			t++;
			if (*t !=' ') {
				*p = *t;
				*t = ' ';
			}
		}else if (*p == '"' ) {
			t++;
			if (*t !='"') {
				*p = *t;
				*t = '"';
			}
		}else if (*p == ',' ) {
			t++;
			if (*t !=',') {
				*p = *t;
				*t = ',';
			}
		}else if (*p == '\n' ) {
			t++;
			if (*t != '\n') 
				*p = *t;
		}else
		{
			p++;
			t = p;
		}
	}
}
			
int vol_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{

    /* example showing pulling the argument values out of the argv array */
    printf("hfeng %s %d- %s <- i:%d\n\n",__func__,__LINE__, path, argv[0]->i);

    fflush(stdout);

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");
    fflush(stdout);

    return 0;
}

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

static int parse_xml_parameter()
{
	xmlDocPtr doc;  
    xmlNodePtr curNode;  
  
    xmlKeepBlanksDefault(0);  
    doc = xmlReadFile("mine.xml", "UTF-8", XML_PARSE_RECOVER); // open a xml doc.  
    curNode = xmlDocGetRootElement(doc); // get root element.  
}
