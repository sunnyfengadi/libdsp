/*
 * =====================================================================================
 *
 *       Filename:  adi_osc.h
 *
 * =====================================================================================
 */

#ifndef ADI_OSC_H
#define ADI_OSC_H

#define DOMAIN 0
#define MASTER_NODE_NUM 0 
#define MASTER_PORT_NUM 101
#define SLAVE_NODE_NUM 1
#define SLAVE_PORT_NUM 5

void error(int num, const char *m, const char *path);
//int generic_handler(const char *path, const char *types, lo_arg **argv,
//		    int argc, void *data, void *user_data);
//int user_widget_handler(const char *path, const char *types, lo_arg **argv,
//		    int argc, void *data, void *user_data);
int arm_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data);
int dsp_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data);
//void state_parse_handler();	    
//void delSpace(char *str);
//static void arm_control_ops(const char *path, lo_type type,
//	lo_arg *argv, void *user_data);
//static void musicplay_control_ops(const char *path, lo_type type,
//	lo_arg *argv, void *user_data);
//static void echo_loopback_control_ops(const char *path, lo_type type,
//	lo_arg *argv, void *user_data);
//static void pot_hadc_loopback_control_ops(const char *path, lo_type type,
//	lo_arg *argv, void *user_data);
//static void dsp_control_ops(const char *path, lo_type type,
//	lo_arg *argv, void *user_data);

 
struct osc_ops{
	char osc_id[50];
	float osc_value;
	lo_type type;
    char *osc_path;
	char *amixer_name;
    void (*do_audio_control)(const char *path, const char *types, lo_arg *argv, void *user_data);
};

#if 0
typedef struct widget{
	ctr_type ctr_type;	/* DSP/ARM control the audio*/
	lo_type type;		/* Widget type */
	char *osc_path;		/* Widget address in Open Stage Control page */
//	char *addr;			/* the address of the audio_effect_param,
//						 * indicate the parameter should be parsed */
	char *desc;		/* description for the widget */
};
#endif

typedef union {
    int32_t  i;
    float    f;
    char     c;
    uint32_t nl;
} lo_pcast32;
    
typedef union {
    int64_t    i;
    double     f;
    uint64_t   nl;
    lo_timetag tt;
} lo_pcast64;

/* TODO add your own OSC widget here */

/* amixer controls names, users should not change this */
#define AMIXER_ADC_BIAS						"ADC Bias"
#define AMIXER_ADC_HIGH_PASS_FILTER_SWITCH	"ADC High Pass Filter Switch"
#define AMIXER_AUX_CAPTURE_VOLUME			"Aux Capture Volume"
#define AMIXER_CAPTURE_BIAS					"Capture Bias"
#define AMIXER_CAPTURE_BOOSE				"Capture Boost"
#define AMIXER_CAPTURE_MUX					"Capture Mux"
#define AMIXER_DAC_BIAS						"DAC Bias"
#define AMIXER_DAC_MONO_STEREO_MODE			"DAC Mono-Stereo-Mode"
#define AMIXER_DAC_PLAYBACK_MUX				"DAC Playback Mux"
#define AMIXER_DIGITAL_CAPTURE_VOLUME		"Digital Capture Volume"
#define AMIXER_DIGITAL_PLAYBACK_VOLUME		"Digital Playback Volume"
#define AMIXER_HEADPHONE_BIAS				"Headphone Bias"
#define AMIXER_HEADPHONE_PLAYBACK_SWITCH	"Headphone Playback Switch"
#define AMIXER_HEADPHONE_PLAYBACK_VOLUME	"Headphone Playback Volume"
#define AMIXER_INPUT_1_CAPTURE_VOLUME		"Input 1 Capture Volume"
#define AMIXER_INPUT_2_CAPTURE_VOLUME		"Input 2 Capture Volume"
#define AMIXER_INPUT_3_CAPTURE_VOLUME		"Input 3 Capture Volume"
#define AMIXER_INPUT_4_CAPTURE_VOLUME		"Input 4 Capture Volume"
#define AMIXER_LEFT_LR_PLAYBACK_MIXER_LEFT_VOLUME \
			"Left LR Playback Mixer Left Volume"
#define AMIXER_LEFT_LR_PLAYBACK_MIXER_RIGHT_VOLUME \
			"Left LR Playback Mixer Right Volume"
#define AMIXER_LEFT_PLAYBACK_MIXER_AUX_BYPASS_VOLUME \
			"Left Playback Mixer Aux Bypass Volume"
#define AMIXER_LEFT_PLAYBACK_MIXER_LEFT_BYPASS_VOLUME \
			"Left Playback Mixer Left Bypass Volume"
#define AMIXER_LEFT_PLAYBACK_MIXER_LEFT_DAC_SWITCH \
			"Left Playback Mixer Left DAC Switch"
#define AMIXER_LEFT_PLAYBACK_MIXER_RIGHT_BYPASS_VOLUME \
			"Left Playback Mixer Right Bypass Volume"
#define AMIXER_LEFT_PLAYBACK_MIXER_RIGHT_DAC_SWITCH \
			"Left Playback Mixer Right DAC Switch"
#define AMIXER_LINEOUT_PLAYBACK_SWITCH		"Lineout Playback Switch"
#define AMIXER_LINEOUT_PLAYBACK_VOLUME		"Lineout Playback Volume"
#define AMIXER_MIC_BIAS_MODE				"Mic Bias Mode"
#define AMIXER_MONO_PLAYBACK_SWITCH			"Mono Playback Switch"
#define AMIXER_MONO_PLAYBACK_VOLUME			"Mono Playback Volume"
#define AMIXER_PLAYBACK_BIAS				"Playback Bias"
#define AMIXER_PLAYBACK_DE_EMPHASIS_SWITCH	"Playback De-emphasis Switch"
#define AMIXER_RIGHT_LR_PLAYBACK_MIXER_LEFT_VOLUME \
			"Right LR Playback Mixer Left Volume"
#define AMIXER_RIGHT_LR_PLAYBACK_MIXER_RIGHT_VOLUME \
			"Right LR Playback Mixer Right Volume"
#define AMIXER_RIGHT_PLAYBACK_MIXER_AUX_BYPASS_VOLUME \
			"Right Playback Mixer Aux Bypass Volume"
#define AMIXER_RIGHT_PLAYBACK_MIXER_LEFT_BYPASS_VOLUME \
			"Right Playback Mixer Left Bypass Volume"
#define AMIXER_RIGHT_PLAYBACK_MIXER_LEFT_DAC_SWITCH \
			"Right Playback Mixer Left DAC Switch"
#define AMIXER_RIGHT_PLAYBACK_MIXER_RIGHT_BYPASS_VOLUME \
			"Right Playback Mixer Right Bypass Volume",
#define AMIXER_RIGHT_PLAYBACK_MIXER_RIGHT_DAC_SWITCH \
			"Right Playback Mixer Right DAC Switch"

#endif
