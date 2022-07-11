#include <stdio.h>
#include "audioPlayer.h"

#include "esp_log.h"

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "filter_resample.h"
#include "equalizer.h"

#include "audio_event_iface.h"
#include "periph_wifi.h"

#include "stateConfig.h"

#include "board.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
static const char *TAG = "AUDIO";

audio_pipeline_handle_t pipeline;
audio_board_handle_t board_handle;
audio_element_handle_t i2s_stream_writer, mp3_decoder, fatfs_stream_reader, rsp_handle, equalizer;
char *url = NULL;
//playlist_operator_handle_t sdcard_list_handle = NULL;

audio_event_iface_handle_t evt;

extern stateStruct monofon_state;
extern configuration monofon_config;


void audioInit(void) {
	uint32_t startTick = xTaskGetTickCount();
	uint32_t heapBefore = xPortGetFreeHeapSize();
	ESP_LOGD(TAG, "Start codec chip");

	board_handle = audio_board_init();
	audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

	ESP_LOGD(TAG, "Create audio pipeline for playback");
	audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
	pipeline_cfg.rb_size = 4*1024;
	pipeline = audio_pipeline_init(&pipeline_cfg);
	mem_assert(pipeline);

	ESP_LOGD(TAG, "Create i2s stream to write data to codec chip");
	i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
	i2s_cfg.i2s_config.sample_rate = 48000;
	i2s_cfg.i2s_config.dma_buf_count = 3; //3
	i2s_cfg.i2s_config.dma_buf_len = 300; //300
	i2s_cfg.type = AUDIO_STREAM_WRITER;
	i2s_cfg.task_prio = 23; //23
	i2s_cfg.use_alc = true;
	i2s_cfg.volume = -34 + (monofon_config.volume / 3);
	i2s_stream_writer = i2s_stream_init(&i2s_cfg);

	fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
	fatfs_cfg.type = AUDIO_STREAM_READER;
	fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);

	ESP_LOGD(TAG, "Create mp3 decoder to decode mp3 file");
	mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
	mp3_decoder = mp3_decoder_init(&mp3_cfg);

	ESP_LOGD(TAG, "Create resample filter");
	rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
	rsp_cfg.prefer_flag = 1;
	rsp_handle = rsp_filter_init(&rsp_cfg);

	ESP_LOGD(TAG, "Register all elements to audio pipeline");
	audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
	audio_pipeline_register(pipeline, mp3_decoder, "mp3");
	audio_pipeline_register(pipeline, rsp_handle, "filter");

	audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

	ESP_LOGD(TAG, "Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->resample-->i2s_stream-->[codec_chip]");
	const char *link_tag[4] = { "file", "mp3", "filter", "i2s" };
	audio_pipeline_link(pipeline, &link_tag[0], 4);

	ESP_LOGD(TAG, "Set up  event listener");
	audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
	evt_cfg.external_queue_size = 5;
	evt_cfg.internal_queue_size = 5;
	evt_cfg.queue_set_size = 5;
	evt = audio_event_iface_init(&evt_cfg);

	ESP_LOGD(TAG, "Listen for all pipeline events");
	audio_pipeline_set_listener(pipeline, evt);


	ESP_LOGD(TAG, "Audio init complite. Duration: %d ms. Heap usage: %d free Heap:%d", (xTaskGetTickCount() - startTick) * portTICK_RATE_MS, heapBefore - xPortGetFreeHeapSize(),xPortGetFreeHeapSize());
}

void audioDeinit(void) {
	audio_pipeline_unregister(pipeline, mp3_decoder);
	audio_pipeline_unregister(pipeline, i2s_stream_writer);
	audio_pipeline_unregister(pipeline, rsp_handle);

	audio_pipeline_deinit(pipeline);
	//audio_element_deinit(fatfs_stream_reader);
	audio_element_deinit(i2s_stream_writer);
	audio_element_deinit(mp3_decoder);
	audio_element_deinit(rsp_handle);
}

void audioPlay(void) {
	uint32_t heapBefore = xPortGetFreeHeapSize();

	audio_element_info_t music_info = { 0 };
	audio_element_getinfo(mp3_decoder, &music_info);
	ESP_LOGD(TAG, "Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d", music_info.sample_rates, music_info.bits, music_info.channels);
	//i2s_alc_volume_set(i2s_stream_writer,monofon_config.volume) ;
	audio_element_setinfo(i2s_stream_writer, &music_info);
	rsp_filter_set_src_info(rsp_handle, music_info.sample_rates, music_info.channels);
	//url = "/sdcard/monofonRus.mp3";
	//audio_hal_set_volume(board_handle->audio_hal, monofon_config.volume);
	//i2s_alc_volume_set(audio_element_handle_t i2s_stream, int volume);

	ESP_LOGD(TAG, "Set volume: %d", monofon_config.volume);
	audio_element_set_uri(fatfs_stream_reader, monofon_config.lang[monofon_state.currentLang].audioFile);
	//audio_pipeline_reset_ringbuffer(pipeline);
	//audio_pipeline_reset_elements(pipeline);
	//audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
	audio_pipeline_run(pipeline);

	ESP_LOGD(TAG, "Start playing file:%s Heap usage:%d, Free heap:%d",monofon_config.lang[monofon_state.currentLang].audioFile, heapBefore - xPortGetFreeHeapSize(), xPortGetFreeHeapSize());
}

void audioStop(void) {
	//audio_pipeline_pause(pipeline);
	//ESP_ERROR_CHECK(audio_pipeline_stop(pipeline));
	audio_pipeline_stop(pipeline);
	audio_pipeline_wait_for_stop(pipeline);
	ESP_ERROR_CHECK(audio_pipeline_terminate(pipeline));
	ESP_ERROR_CHECK(audio_pipeline_reset_ringbuffer(pipeline));
	ESP_ERROR_CHECK(audio_pipeline_reset_elements(pipeline));
	ESP_ERROR_CHECK(audio_pipeline_change_state(pipeline, AEL_STATE_INIT));

	ESP_LOGD(TAG, "Stop playing. Free heap:%d", xPortGetFreeHeapSize());
}

void audioPause(void) {

	ESP_LOGD(TAG, "Pausing audio pipeline");
	audio_pipeline_pause(pipeline);
}

void listenListener(void *pvParameters) {
	while(1){
		vTaskDelay(pdMS_TO_TICKS(10));

		audio_event_iface_msg_t msg;
		esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
		if(ret!=ESP_OK){
			ESP_LOGD(TAG, "audio_event_iface_listen FAIL: %d", ret);
		}else{
			ESP_LOGD(TAG, "audio_event_iface_listen source_type: %d  msg_cmd:%d", msg.source_type, msg.cmd);
		}
	}


}
