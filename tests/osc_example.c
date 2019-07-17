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
#include <unistd.h>

#include <alsa/asoundlib.h>
#include "dsp_communication.h"

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


/* ARM Control */
int done = 0;
int fd = 0;
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
};
static struct osc_ops amixer_audio_ops[] = {
};
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

#if 0
static void dsp_control_ops2(const char *path, const char *types,
			lo_arg *argv, void *user_data)
{
	if(strcmp(path, OSC_DSP_GAIN_DELAY_SECONDS) == 0 ||
		strcmp(path, OSC_DSP_AUDIOPROJ_FIN_POT_HADC2) == 0	) 	
	    effect_param.audioproj_fin_pot_hadc2 = argv->f;
	else if(strcmp(path, OSC_DSP_GAIN_WET_MIX) == 0 ||
		strcmp(path, OSC_DSP_AUDIOPROJ_FIN_POT_HADC1) == 0) 	
	    effect_param.audioproj_fin_pot_hadc1 = argv->f;
	else if(strcmp(path, OSC_DSP_GAIN_FEEDBACK)== 0 || 	
		strcmp(path, OSC_DSP_AUDIOPROJ_FIN_POT_HADC0) == 0) 	
	    effect_param.audioproj_fin_pot_hadc0 = argv->f;
	
	if(strcmp(path, OSC_DSP_EFFECTS_PRESET) == 0)
	    effect_param.effects_preset = argv->i;

	if(strcmp(path, OSC_DSP_REVERB_PRESET) == 0)
	    effect_param.reverb_preset = argv->i;

    write(fd, (char *)&effect_param, sizeof(EFFECT_PARAM)); 
	printf("hfeng effects_preset=%d\n",effect_param.effects_preset);
	printf("hfeng reverb_preset=%d\n",effect_param.reverb_preset);
	printf("hfeng hadc0=%f\n",effect_param.audioproj_fin_pot_hadc0);
	printf("hfeng hadc1=%f\n",effect_param.audioproj_fin_pot_hadc1);
	printf("hfeng hadc2=%f\n",effect_param.audioproj_fin_pot_hadc2);
}
#endif

static void dsp_control_ops(const char *path, lo_type type,
			lo_arg *argv, void *user_data)
{
	int id = *(int32_t *)user_data;
    lo_pcast32 val32;
    lo_pcast64 val64;
    int size;
    int i;
	int bigendian = 0;
	void *data = (void *)argv;
	char * sram_base ;
	char send_buf[32] = "";
	char * sram_vaddr;

	printf("\nhfeng%s %d path=%s\n",__func__,__LINE__, path);
	printf("%s fdfdfd =0x%x\n",__func__,dsp_fd);

	/* DSP echo switch control */
//	char startup_cmd[40];
//	char stop_cmd[40];
//	sprintf(startup_cmd, "sh /%s_startup.sh","echo"); 

	/* process path value */
	printf("\nhfeng%s %d process path value \n",__func__,__LINE__);
    size = lo_arg_size(type, data);
    if (size == 4 || type == LO_BLOB) {
		if (bigendian) {
			val32.nl = lo_otoh32(*(int32_t *)data);
		} else {
		    val32.nl = *(int32_t *)data;
		}
    } else if (size == 8) {
		if (bigendian) {
			val64.nl = lo_otoh64(*(int64_t *)data);
		} else {
		    val64.nl = *(int64_t *)data;
		}
    }

//	send_buf.size = size;
//	printf("hfeng send.size=%d\n", send_buf.size);
//	alloc_dsp_com_mem(&master, &send_buf, &sram_vaddr);

//	printf("hfeng alloc_dsp_com_mem finished, vaddr=0x%x, paddr=0x%x,send.size=%d\n",sram_vaddr, send_buf.paddr,send_buf.size);

	/* copy path data into SRAM */
    switch (type) {
    case LO_INT32:
		printf("hfeng %s %d val32.i=%d\n",__func__,__LINE__, val32.i);
		dsp_com_write(dsp_fd, &val32.i, size, block_mode); 
//		memcpy(&sram_vaddr, &val32.i, sizeof(user_wid[id].type));
//		printf("hfeng effects_preset = %d\n", effect_param.effects_preset);
//		printf("hfeng size= %d\n",sizeof(user_wid[id].type));
		break;
    case LO_FLOAT:
		printf("hfeng %s %d val32.f=%f\n",__func__,__LINE__, val32.f);
		dsp_com_write(dsp_fd, &val32.f, size, block_mode); 
//		memcpy(&sram_vaddr, &val32.f, sizeof(user_wid[id].type));
//		printf("hfeng audioproj_fin_pot_hadc2 = %f\n", effect_param.audioproj_fin_pot_hadc2);
//		printf("hfeng size= %d\n",sizeof(user_wid[id].type));
		break;
    default:
		fprintf(stderr, "liblo warning: unhandled type: %c\n", type);
		break;
    }

	/* send buffer addr+size to DSP */
//	send_buf.size = sizeof(user_wid[id].type);

//	printf("hfeng l2addr=0x%x, size=%d\n",send_buf.paddr, send_buf.size);

//	char buf[]="0x20084000";
//	free_dsp_com_mem(&send_buf, );


  //  write(fd, (char *)&effect_param, sizeof(EFFECT_PARAM)); 
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

int main()
{
    int err = 0;
	int fd;
//	state_parse_handler();
#if 0
    fd = open("/sys/audio_dsp_delay/audio_dsp_delay", O_RDWR);
    if (fd < 0){
		printf("open file error\n");
		return -1;
	}
#endif
	/*  Prepare for DSP control ops */
	printf("hfeng %s &fdfdfd =0x%x\n",__func__,dsp_fd);

	dsp_fd = dsp_com_open(connection_id, MAX_BUF_SIZE, block_mode);
	printf("hfeng %s &fdfdfd =0x%x\n",__func__,dsp_fd);

	//	printf("hfeng222 -dsp_com_opened!(%d,%d,%d)to(%d,%d,%d)\n",
	//		(dsp_handle.master.domain),
	//		(dsp_handle.master.node),
	//		(dsp_handle.master.port),
	//		(dsp_handle.slave.domain),
	//		(dsp_handle.slave.node),
	//		(dsp_handle.slave.port));
	//printf("hfeng %s %d sram_handle=%ld",__func__,__LINE__, dsp_handle.sram_handle);

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


    /* start a new server on port 7770 */
    lo_server_thread st = lo_server_thread_new("7770", error);

    /* add method that will match any path and args */
//    lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);
    lo_server_thread_add_method(st, NULL, NULL, user_widget_handler, NULL);

    /* add method that will match the path /foo/bar
     * to float and int */
//    lo_server_thread_add_method(st, "/update/state", NULL, state_parse_handler, NULL);
    

    /* add method that will match the path /quit with no args */
//    lo_server_thread_add_method(st, "/quit", "", quit_handler, NULL);

    lo_server_thread_start(st);

    while (!done) {
#ifdef WIN32
    Sleep(1);
#else
	usleep(1000);
#endif
    }

    lo_server_thread_free(st);
	dsp_com_close(dsp_fd, connection_id);

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
