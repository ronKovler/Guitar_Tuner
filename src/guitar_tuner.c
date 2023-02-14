#include "guitar_tuner.h"
#include <audio_io.h>
#include <player.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlog.h>
#include <sound_manager.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include "fast-dct-fft.h"


#define NUM_SAMPLES 		16344
#define SAMPLE_RATE 		48000
#define CLOCK_TIME_INTERVAL	0.50
#define AUDIO_TIME_INTERVAL 0.3404999
#define SAMPLE_FFT			4096

int bufferSize;


typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *label;
	Evas_Object *button;
	Evas_Object *layout;
	Evas_Object *grid;
	Evas_Object *table;
	audio_in_h input;

	Evas_Object *label2;

	Ecore_Timer *audio_timer;

} appdata_s;

void *buffer;
double *fftBuffer;
double resolution;



static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}



static void button_clicked_cb(void *data, Evas_Object *button, void *event_info) {
	appdata_s *ad = data;

	ecore_timer_thaw(ad->audio_timer);



}

Eina_Bool audio_timer_cb(void *data) {
	appdata_s *ad = data;
	int aret;
	aret = audio_in_prepare(ad->input);
	if (aret != 0) {
		dlog_print(DLOG_ERROR, "Error", "%s", "Failed to prepare audio channel.\n");
	}

	unsigned int num =  (unsigned int) NUM_SAMPLES;
	num *= 2;

	aret = audio_in_read(ad->input, buffer, num);
	if (aret != 0) {
		dlog_print(DLOG_ERROR, "Error", "%s", "Failed to read audio in channel.\n");
	}

	audio_in_unprepare(ad->input);
//	audio_in_flush(ad->input);


	short *bufferShort = (short *) buffer;


	for (int i = 0; i < NUM_SAMPLES; i++) {
		fftBuffer[i] = (double) bufferShort[i];
	}


	FastDctFft_transform(fftBuffer, NUM_SAMPLES);

	double max = fftBuffer[20];
	int maxIdx = 20;
	for (int i = 1; i < 121; i++) {
		if (fabs(fftBuffer[i]) > max) {
			max = fabs(fftBuffer[i]);
			maxIdx = i;
		}
	}

	double starting_frequency = maxIdx * resolution;
	double avg_frequency = (starting_frequency * 2 + resolution) / 2.0;


//	char *bufferChar = (char *) buffer;
//
//	int crossings = 0;
//
//	short prev = bufferShort[0];
//	bufferShort++;
//
//	while ((char *) bufferShort < bufferChar + (NUM_SAMPLES * 2)) {
//		short cur = *bufferShort;
//		if (cur * prev < 0) {
//			crossings++;
//		}
//
//		prev = cur;
//		bufferShort++;
//	}
//
//	double time_elapsed = AUDIO_TIME_INTERVAL;
//	int cycles1 = (crossings - 1) / 1;
//	int cycles2 = (crossings - 1) / 2;
//
//	double freq1 = cycles1 / time_elapsed;
//	double freq2 = cycles2 / time_elapsed;




	char buf[100] = {0};
	char buf2[100] = {0};

	sprintf(buf, "<align=center>%.2lf Hz</align>", avg_frequency);
	sprintf(buf2, "<align=center>R %d byts</align>", aret);

	//sprintf(buf, "<align=center>max: %d, ret: %d</align>", maxIdx, aret);
	evas_object_hide(ad->label);
	elm_object_text_set(ad->label, buf);
	evas_object_show(ad->label);

	evas_object_hide(ad->label2);
	elm_object_text_set(ad->label2, buf2);
	evas_object_show(ad->label2);









	return ECORE_CALLBACK_RENEW;
}


static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	/* Create and initialize elm_win.
	   elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	/* Create and initialize elm_conformant.
	   elm_conformant is mandatory for base gui to have proper size
	   when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);



//	ad->button = elm_button_add(ad->win);
//	elm_object_text_set(ad->button, "<align=center>Start</align>");
//	elm_object_style_set(ad->button, "default");
//	evas_object_smart_callback_add(ad->button, "clicked", button_clicked_cb, ad);
//
//	evas_object_show(ad->button);
	//ad->layout = elm_layout

	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	ad->grid = elm_grid_add(ad->conform);
	evas_object_size_hint_weight_set(ad->grid,EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->grid,EVAS_HINT_FILL,EVAS_HINT_FILL);
	elm_object_content_set(ad->conform,ad->grid);

	ad->table = elm_table_add(ad->grid);
	elm_table_padding_set(ad->table,10,10);
	elm_grid_pack(ad->grid,ad->table,5,15,50,50);

	ad->button = elm_button_add(ad->table);
	evas_object_size_hint_weight_set(ad->button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->button, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_text_set(ad->button, "<font_size = 50>Start!</font_size>");
	evas_object_show(ad->button);
	evas_object_smart_callback_add(ad->button,"clicked",button_clicked_cb, ad);
	elm_table_pack(ad->table, ad->button, 0, 0, 1, 1);

	/* Label */
	/* Create an actual view of the base gui.
	   Modify this part to change the view. */
	ad->label = elm_label_add(ad->table);
	elm_object_text_set(ad->label, "<align=center>0</align>");
	evas_object_size_hint_weight_set(ad->label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(ad->label);
	elm_table_pack(ad->table, ad->label, 0, 1, 1, 1);

	ad->label2 = elm_label_add(ad->table);
	elm_object_text_set(ad->label2, "<align=center>0</align>");
	evas_object_size_hint_weight_set(ad->label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(ad->label2);
	elm_table_pack(ad->table, ad->label2, 0, 2, 1, 1);






	evas_object_show(ad->table);




	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}


static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);
	int aret;
	aret = audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_S16_LE, &ad->input);

	aret = audio_in_get_buffer_size(ad->input, &bufferSize);
	//audio_in_set_stream_cb(ad->input, audio_stream_cb, ad);

	ad->audio_timer = ecore_timer_add(CLOCK_TIME_INTERVAL, audio_timer_cb, ad);
	ecore_timer_freeze(ad->audio_timer);
	ecore_timer_precision_set(0.01);


	buffer = malloc(1000000);
	fftBuffer = malloc(NUM_SAMPLES * sizeof(double));
	resolution = SAMPLE_RATE / NUM_SAMPLES;


	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);


	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
