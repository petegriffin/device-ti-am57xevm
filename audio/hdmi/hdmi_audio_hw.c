/* -*- mode: C; c-file-style: "stroustrup"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2012 Texas Instruments
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "hdmi_audio_hw"
/* #define LOG_NDEBUG 0 */
/* #define LOG_TRACE_FUNCTION */

#ifndef LOG_TRACE_FUNCTION
#define TRACE() ((void)0)
#define TRACEM(fmt, ...) ((void)0)
#else
#define tfmt(x) x
#define TRACE() (ALOGV("%s() %s:%d", __func__, __FILE__, __LINE__))
#define TRACEM(fmt, ...) (ALOGV("%s() " tfmt(fmt) " %s:%d", __func__, ##__VA_ARGS__, __FILE__, __LINE__))
#endif

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <tinyalsa/asoundlib.h>

#include <OMX_Audio.h>

#include "hdmi_audio_hal.h"

#define UNUSED(x) (void)(x)

#define MAX_CARD_COUNT 10
#define HDMI_PCM_DEV 0
#define HDMI_SAMPLING_RATE 44100
#define HDMI_PERIOD_SIZE 1920
#define HDMI_PERIOD_COUNT 4
#define HDMI_MAX_CHANNELS 8

typedef struct _hdmi_device {
    audio_hw_device_t device;
    int map[HDMI_MAX_CHANNELS];
    bool CEAMap;
} hdmi_device_t;

int cea_channel_map[HDMI_MAX_CHANNELS] = {OMX_AUDIO_ChannelLF,OMX_AUDIO_ChannelRF,OMX_AUDIO_ChannelLFE,
        OMX_AUDIO_ChannelCF,OMX_AUDIO_ChannelLS,OMX_AUDIO_ChannelRS,
        OMX_AUDIO_ChannelLR,OMX_AUDIO_ChannelRR};  /*Using OMX_AUDIO_CHANNELTYPE mapping*/

typedef struct _hdmi_out {
    audio_stream_out_t stream_out;
    hdmi_device_t *dev;
    struct pcm_config config;
    struct pcm *pcm;
    audio_config_t android_config;
    int up;
    void *buffcpy;
} hdmi_out_t;

#define S16_SIZE sizeof(int16_t)


/*****************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************
 */

/*****************************************************************
 * AUDIO STREAM OUT (hdmi_out_*) DEFINITION
 *****************************************************************
 */

uint32_t hdmi_out_get_sample_rate(const struct audio_stream *stream)
{
    hdmi_out_t *out = (hdmi_out_t*)stream;
    struct pcm_config *config = &out->config;
    TRACEM("stream=%p returning %d", stream, config->rate);
    return config->rate;
}

/* DEPRECATED API */
int hdmi_out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    TRACE();
    UNUSED(stream);
    UNUSED(rate);

    return -EINVAL;
}

/* Returns bytes for ONE PERIOD */
size_t hdmi_out_get_buffer_size(const struct audio_stream *stream)
{
    const struct audio_stream_out *s = (const struct audio_stream_out *)stream;
    hdmi_out_t *out = (hdmi_out_t*)stream;
    struct pcm_config *config = &out->config;
    size_t ans;

    ans = audio_stream_out_frame_size(s) * config->period_size;

    TRACEM("stream=%p returning %u", stream, ans);

    return ans;
}

audio_channel_mask_t hdmi_out_get_channels(const struct audio_stream *stream)
{
    hdmi_out_t *out = (hdmi_out_t*)stream;
    TRACEM("stream=%p returning %x", stream, out->android_config.channel_mask);
    return out->android_config.channel_mask;
}

audio_format_t hdmi_out_get_format(const struct audio_stream *stream)
{
    hdmi_out_t *out = (hdmi_out_t*)stream;
    TRACEM("stream=%p returning %x", stream, out->android_config.format);
    return out->android_config.format;
}

/* DEPRECATED API */
int hdmi_out_set_format(struct audio_stream *stream, audio_format_t format)
{
    TRACE();
    UNUSED(stream);
    UNUSED(format);

    return -EINVAL;
}

int hdmi_out_standby(struct audio_stream *stream)
{
    hdmi_out_t *out = (hdmi_out_t*)stream;

    TRACEM("stream=%p", stream);

    if (out->up && out->pcm) {
        out->up = 0;
        pcm_close(out->pcm);
        out->pcm = 0;
    }

    return 0;
}

int hdmi_out_dump(const struct audio_stream *stream, int fd)
{
    TRACE();
    UNUSED(stream);
    UNUSED(fd);

    return 0;
}

audio_devices_t hdmi_out_get_device(const struct audio_stream *stream)
{
    TRACEM("stream=%p", stream);
    UNUSED(stream);

    return AUDIO_DEVICE_OUT_AUX_DIGITAL;
}

/* DEPRECATED API */
int hdmi_out_set_device(struct audio_stream *stream, audio_devices_t device)
{
    TRACE();
    UNUSED(stream);
    UNUSED(device);

    return 0;
}

int hdmi_out_set_parameters(struct audio_stream *stream, const char *kv_pairs)
{
    TRACEM("stream=%p kv_pairs='%s'", stream, kv_pairs);
    UNUSED(stream);
    UNUSED(kv_pairs);

    return 0;
}

#define MASK_CEA_QUAD     ( CEA_SPKR_FLFR | CEA_SPKR_RLRR )
#define MASK_CEA_SURROUND ( CEA_SPKR_FLFR | CEA_SPKR_FC | CEA_SPKR_RC )
#define MASK_CEA_5POINT1  ( CEA_SPKR_FLFR | CEA_SPKR_FC | CEA_SPKR_LFE | CEA_SPKR_RLRR )
#define MASK_CEA_7POINT1  ( CEA_SPKR_FLFR | CEA_SPKR_FC | CEA_SPKR_LFE | CEA_SPKR_RLRR | CEA_SPKR_RLCRRC )
#define SUPPORTS_ARR(spkalloc, profile) (((spkalloc) & (profile)) == (profile))

char * hdmi_out_get_parameters(const struct audio_stream *stream,
			 const char *keys)
{
    struct str_parms *query = str_parms_create_str(keys);
    char *str;
    char value[256];
    struct str_parms *reply = str_parms_create();
    int status;
    hdmi_audio_caps_t caps;

    TRACEM("stream=%p keys='%s'", stream, keys);
    UNUSED(stream);

    if (hdmi_query_audio_caps(&caps)) {
        ALOGE("Unable to get the HDMI audio capabilities");
        str = calloc(1, 1);
        goto end;
    }

    status = str_parms_get_str(query, AUDIO_PARAMETER_STREAM_SUP_CHANNELS,
                                value, sizeof(value));
    if (status >= 0) {
        unsigned sa = caps.speaker_alloc;
        bool first = true;

        /* STEREO is intentionally skipped.  This code is only
         * executed for the 'DIRECT' interface, and we don't
         * want stereo on a DIRECT thread.
         */
        value[0] = '\0';
        if (SUPPORTS_ARR(sa, MASK_CEA_QUAD)) {
            if (!first) {
                strcat(value, "|");
            }
            first = false;
            strcat(value, "AUDIO_CHANNEL_OUT_QUAD");
        }
        if (SUPPORTS_ARR(sa, MASK_CEA_SURROUND)) {
            if (!first) {
                strcat(value, "|");
            }
            first = false;
            strcat(value, "AUDIO_CHANNEL_OUT_SURROUND");
        }
        if (SUPPORTS_ARR(sa, MASK_CEA_5POINT1)) {
            if (!first) {
                strcat(value, "|");
            }
            first = false;
            strcat(value, "AUDIO_CHANNEL_OUT_5POINT1");
        }
        if (SUPPORTS_ARR(sa, MASK_CEA_7POINT1)) {
            if (!first) {
                strcat(value, "|");
            }
            first = false;
            strcat(value, "AUDIO_CHANNEL_OUT_7POINT1");
        }
        str_parms_add_str(reply, AUDIO_PARAMETER_STREAM_SUP_CHANNELS, value);
        str = strdup(str_parms_to_str(reply));
    } else {
        str = strdup(keys);
    }

    ALOGV("%s() reply: '%s'", __func__, str);

end:
    str_parms_destroy(query);
    str_parms_destroy(reply);
    return str;
}
int hdmi_out_add_audio_effect(const struct audio_stream *stream,
			effect_handle_t effect)
{
    TRACE();
    UNUSED(stream);
    UNUSED(effect);

    return 0;
}
int hdmi_out_remove_audio_effect(const struct audio_stream *stream,
			   effect_handle_t effect)
{
    TRACE();
    UNUSED(stream);
    UNUSED(effect);

    return 0;
}

/* returns milliseconds */
uint32_t hdmi_out_get_latency(const struct audio_stream_out *stream)
{
    uint32_t latency;
    hdmi_out_t *out = (hdmi_out_t*)stream;
    struct pcm_config *config = &out->config;

    TRACEM("stream=%p", stream);

    return (1000 * config->period_size * config->period_count) / config->rate;
}

int hdmi_out_set_volume(struct audio_stream_out *stream, float left, float right)
{
    TRACE();
    UNUSED(stream);
    UNUSED(left);
    UNUSED(right);

    return -ENOSYS;
}

static int hdmi_out_find_card(void)
{
    struct mixer *mixer;
    const char *name;
    int card = 0;
    int found = 0;
    int i;

    do {
        /* returns an error after last valid card */
        mixer = mixer_open(card);
        if (!mixer)
            break;

        name = mixer_get_name(mixer);

        if (strstr(name, "HDMI") || strstr(name, "hdmi")) {
            TRACEM("HDMI card '%s' found at %d", name, card);
            found = 1;
        }

        mixer_close(mixer);
    } while (!found && (card++ < MAX_CARD_COUNT));

    /* Use default card number if not found */
    if (!found)
        card = 1;

    return card;
}

static int hdmi_out_open_pcm(hdmi_out_t *out)
{
    int card = hdmi_out_find_card();
    int dev = HDMI_PCM_DEV;
    int ret;

    TRACEM("out=%p", out);

    /* out->up must be 0 (down) */
    if (out->up) {
        ALOGE("Trying to open a PCM that's already up. "
             "This will probably deadlock... so aborting");
        return 0;
    }

    out->pcm = pcm_open(card, dev, PCM_OUT, &out->config);

    if(out->pcm && pcm_is_ready(out->pcm)) {
        out->up = 1;
        ret = 0;
    } else {
        ALOGE("cannot open HDMI pcm card %d dev %d error: %s",
              card, dev, pcm_get_error(out->pcm));
        pcm_close(out->pcm);
        out->pcm = 0;
        out->up = 0;
        ret = 1;
    }

    return ret;
}

void channel_remap(struct audio_stream_out *stream, const void *buffer,
                    size_t bytes)
{
        hdmi_out_t *out = (hdmi_out_t*)stream;
        hdmi_device_t *adev = out->dev;
        int x, y, frames;
        int16_t *buf = (int16_t *)buffer;
        int16_t *tmp_buf = (int16_t *)out->buffcpy;

        frames = bytes / audio_stream_out_frame_size(stream);
        while (frames--){
            for(y = 0; y < (int)out->config.channels; y++){
                for(x = 0; x < (int)out->config.channels; x++){
                    if (cea_channel_map[y] == adev->map[x]){
                        tmp_buf[y] = buf[x];
                        break;
                    }
                }
            }
            tmp_buf += (int)out->config.channels;
            buf += (int)out->config.channels;
        }
}

ssize_t hdmi_out_write(struct audio_stream_out *stream, const void* buffer,
		 size_t bytes)
{
    hdmi_out_t *out = (hdmi_out_t*)stream;
    hdmi_device_t *adev = out->dev;
    ssize_t ret;

    TRACEM("stream=%p buffer=%p bytes=%d", stream, buffer, bytes);

    if (!out->up) {
        if(hdmi_out_open_pcm(out)) {
            ret = -ENOSYS;
	    goto exit;
        }
    }

    if (out->config.channels > 2 && !adev->CEAMap){
        channel_remap(stream, buffer, bytes);
        ret = pcm_write(out->pcm, out->buffcpy, bytes);
    } else {
       ret = pcm_write(out->pcm, buffer, bytes);
    }
exit:
    if (ret != 0) {
        ALOGE("Error writing to HDMI pcm: %s",
              out->pcm ? pcm_get_error(out->pcm) : "failed to open PCM device");
        hdmi_out_standby((struct audio_stream*)stream);
	unsigned int usecs = bytes * 1000000 /
                        audio_stream_out_frame_size(stream) /
                        hdmi_out_get_sample_rate((struct audio_stream*)stream);
	if (usecs >= 1000000L) {
	    usecs = 999999L;
	}
	usleep(usecs);
    }

    return bytes;
}


int hdmi_out_get_render_position(const struct audio_stream_out *stream,
			   uint32_t *dsp_frames)
{
    TRACE();
    UNUSED(stream);
    UNUSED(dsp_frames);

    return -EINVAL;
}

int hdmi_out_get_next_write_timestamp(const struct audio_stream_out *stream,
				int64_t *timestamp)
{
    TRACE();
    UNUSED(stream);
    UNUSED(timestamp);

    return -EINVAL;
}



audio_stream_out_t hdmi_stream_out_descriptor = {
    .common = {
        .get_sample_rate = hdmi_out_get_sample_rate,
        .set_sample_rate = hdmi_out_set_sample_rate,
        .get_buffer_size = hdmi_out_get_buffer_size,
        .get_channels = hdmi_out_get_channels,
        .get_format = hdmi_out_get_format,
        .set_format = hdmi_out_set_format,
        .standby = hdmi_out_standby,
        .dump = hdmi_out_dump,
        .get_device = hdmi_out_get_device,
        .set_device = hdmi_out_set_device,
        .set_parameters = hdmi_out_set_parameters,
        .get_parameters = hdmi_out_get_parameters,
        .add_audio_effect = hdmi_out_add_audio_effect,
        .remove_audio_effect = hdmi_out_remove_audio_effect,
    },
    .get_latency = hdmi_out_get_latency,
    .set_volume = hdmi_out_set_volume,
    .write = hdmi_out_write,
    .get_render_position = hdmi_out_get_render_position,
    .get_next_write_timestamp = hdmi_out_get_next_write_timestamp,
};

/*****************************************************************
 * AUDIO DEVICE (hdmi_adev_*) DEFINITION
 *****************************************************************
 */

static int hdmi_adev_close(struct hw_device_t *device)
{
    TRACE();
    UNUSED(device);

    return 0;
}

static uint32_t hdmi_adev_get_supported_devices(const audio_hw_device_t *dev)
{
    TRACE();
    UNUSED(dev);

    return AUDIO_DEVICE_OUT_AUX_DIGITAL;
}

static int hdmi_adev_init_check(const audio_hw_device_t *dev)
{
    TRACE();
    UNUSED(dev);

    return 0;
}

static int hdmi_adev_set_voice_volume(audio_hw_device_t *dev, float volume)
{
    TRACE();
    UNUSED(dev);
    UNUSED(volume);

    return -ENOSYS;
}

static int hdmi_adev_set_master_volume(audio_hw_device_t *dev, float volume)
{
    TRACE();
    UNUSED(dev);
    UNUSED(volume);

    return -ENOSYS;
}

static int hdmi_adev_get_master_volume(audio_hw_device_t *dev, float *volume)
{
    TRACE();
    UNUSED(dev);
    UNUSED(volume);

    return -ENOSYS;
}

static int hdmi_adev_set_mode(audio_hw_device_t *dev, audio_mode_t mode)
{
    TRACE();
    UNUSED(dev);
    UNUSED(mode);

    return 0;
}

static int hdmi_adev_set_mic_mute(audio_hw_device_t *dev, bool state)
{
    TRACE();
    UNUSED(dev);
    UNUSED(state);

    return -ENOSYS;
}

static int hdmi_adev_get_mic_mute(const audio_hw_device_t *dev, bool *state)
{
    TRACE();
    UNUSED(dev);
    UNUSED(state);

    return -ENOSYS;
}

static int hdmi_adev_set_parameters(audio_hw_device_t *dev, const char *kv_pairs)
{
    TRACEM("dev=%p kv_pairss='%s'", dev, kv_pairs);

    struct str_parms *params;
    char *str;
    char value[HDMI_MAX_CHANNELS];
    int ret, x, val, numMatch = 0;
    hdmi_device_t *adev = (hdmi_device_t *)dev;

    params = str_parms_create_str(kv_pairs);
    //Handle maximum of 8 channels
    ret = str_parms_get_str(params, "channel_map", value, HDMI_MAX_CHANNELS);
    if (ret >= 0) {
        val = strtol(value, NULL, 10);
        for(x = 0; x < HDMI_MAX_CHANNELS; x++) {
            adev->map[x] = (val & (0xF << x*4)) >> x*4;
            if (adev->map[x] == cea_channel_map[x])
                numMatch += 1;
        }
        if (numMatch >= 5)
            adev->CEAMap = true;
        else
            adev->CEAMap = false;
    }
    return 0;
}

static char* hdmi_adev_get_parameters(const audio_hw_device_t *dev,
                                      const char *keys)
{
    TRACEM("dev=%p keys='%s'", dev, keys);
    UNUSED(dev);
    UNUSED(keys);

    return NULL;
}

static size_t hdmi_adev_get_input_buffer_size(const audio_hw_device_t *dev,
                                              const struct audio_config *config)
{
    TRACE();
    UNUSED(dev);
    UNUSED(config);

    return 0;
}

#define DUMP_FLAG(flags, flag) {                \
        if ((flags) & (flag)) {                 \
            ALOGV("set:   " #flag);             \
        } else {                                \
            ALOGV("unset: " #flag);             \
        }                                       \
    }

static int hdmi_adev_open_output_stream(audio_hw_device_t *dev,
                                        audio_io_handle_t handle,
                                        audio_devices_t devices,
                                        audio_output_flags_t flags,
                                        struct audio_config *config,
                                        struct audio_stream_out **stream_out,
                                        const char *address)
{
    hdmi_out_t *out = 0;
    struct pcm_config *pcm_config = 0;
    struct audio_config *a_config = 0;

    TRACE();
    UNUSED(handle);
    UNUSED(devices);
    UNUSED(flags);
    UNUSED(address);

    out = calloc(1, sizeof(hdmi_out_t));
    if (!out) {
        return -ENOMEM;
    }

    out->dev = (hdmi_device_t *)dev;
    memcpy(&out->stream_out, &hdmi_stream_out_descriptor,
           sizeof(audio_stream_out_t));
    memcpy(&out->android_config, config, sizeof(audio_config_t));

    pcm_config = &out->config;
    a_config = &out->android_config;

#if defined(LOG_NDEBUG) && (LOG_NDEBUG == 0)
    /* Analyze flags */
    if (flags) {
        DUMP_FLAG(flags, AUDIO_OUTPUT_FLAG_DIRECT)
        DUMP_FLAG(flags, AUDIO_OUTPUT_FLAG_PRIMARY)
        DUMP_FLAG(flags, AUDIO_OUTPUT_FLAG_FAST)
        DUMP_FLAG(flags, AUDIO_OUTPUT_FLAG_DEEP_BUFFER)
    } else {
        ALOGV("flags == AUDIO_OUTPUT_FLAG_NONE (0)");
    }
#endif /* defined(LOG_NDEBUG) && (LOG_NDEBUG == 0) */
    /* Initialize the PCM Configuration */
    pcm_config->period_size = HDMI_PERIOD_SIZE;
    pcm_config->period_count = HDMI_PERIOD_COUNT;

    if (a_config->sample_rate) {
        pcm_config->rate = config->sample_rate;
    } else {
        pcm_config->rate = HDMI_SAMPLING_RATE;
        a_config->sample_rate = HDMI_SAMPLING_RATE;
    }

    switch (a_config->format) {
    case AUDIO_FORMAT_DEFAULT:
        a_config->format = AUDIO_FORMAT_PCM_16_BIT;
        /* fall through */
    case AUDIO_FORMAT_PCM_16_BIT:
        pcm_config->format = PCM_FORMAT_S16_LE;
        break;
    default:
        ALOGE("HDMI rejecting format %x", config->format);
        goto fail;
    }

    a_config->channel_mask = config->channel_mask;
    switch (config->channel_mask) {
    case AUDIO_CHANNEL_OUT_STEREO:
        pcm_config->channels = 2;
        break;
    case AUDIO_CHANNEL_OUT_QUAD:
        pcm_config->channels = 4;
        break;
    case AUDIO_CHANNEL_OUT_5POINT1:
        pcm_config->channels = 6;
        break;
    case AUDIO_CHANNEL_OUT_7POINT1:
        pcm_config->channels = 8;
        break;
    default:
        ALOGE("HDMI setting a default channel_mask %x -> 8", config->channel_mask);
        config->channel_mask = AUDIO_CHANNEL_OUT_7POINT1;
        a_config->channel_mask = AUDIO_CHANNEL_OUT_7POINT1;
        pcm_config->channels = 8;
    }

    //Allocating buffer for at most 8 channels
    out->buffcpy = malloc(pcm_config->period_size * sizeof(int16_t) * HDMI_MAX_CHANNELS);
    if (!out->buffcpy){
        ALOGE("Could not allocate memory");
        goto fail;
    }

    ALOGV("stream = %p", out);
    *stream_out = &out->stream_out;

    return 0;

fail:
    free(out);
    return -ENOSYS;
}

static void hdmi_adev_close_output_stream(audio_hw_device_t *dev,
                                          struct audio_stream_out* stream_out)
{
    hdmi_out_t *out = (hdmi_out_t*)stream_out;

    TRACEM("dev=%p stream_out=%p", dev, stream_out);
    UNUSED(dev);

    stream_out->common.standby((audio_stream_t*)stream_out);
    free(out->buffcpy);
    out->buffcpy = NULL;
    free(stream_out);
}

static int hdmi_adev_open_input_stream(audio_hw_device_t *dev,
                                       audio_io_handle_t handle,
                                       audio_devices_t devices,
                                       struct audio_config *config,
                                       struct audio_stream_in **stream_in,
                                       audio_input_flags_t flags,
                                       const char *address,
                                       audio_source_t source)
{
    TRACE();
    UNUSED(dev);
    UNUSED(handle);
    UNUSED(devices);
    UNUSED(config);
    UNUSED(stream_in);
    UNUSED(flags);
    UNUSED(address);
    UNUSED(source);

    return -ENOSYS;
}

static void hdmi_adev_close_input_stream(audio_hw_device_t *dev,
                                         struct audio_stream_in *stream_in)
{
    TRACE();
    UNUSED(dev);
    UNUSED(stream_in);
}

static int hdmi_adev_dump(const audio_hw_device_t *dev, int fd)
{
    TRACE();
    UNUSED(dev);
    UNUSED(fd);

    return 0;
}

static hdmi_device_t hdmi_adev = {
    .device = {
        .common = {
            .tag = HARDWARE_DEVICE_TAG,
            .version = AUDIO_DEVICE_API_VERSION_2_0,
            .module = NULL,
            .close = hdmi_adev_close,
        },
        .get_supported_devices = hdmi_adev_get_supported_devices,
        .init_check = hdmi_adev_init_check,
        .set_voice_volume = hdmi_adev_set_voice_volume,
        .set_master_volume = hdmi_adev_set_master_volume,
        .get_master_volume = hdmi_adev_get_master_volume,
        .set_mode = hdmi_adev_set_mode,
        .set_mic_mute = hdmi_adev_set_mic_mute,
        .get_mic_mute = hdmi_adev_get_mic_mute,
        .set_parameters = hdmi_adev_set_parameters,
        .get_parameters = hdmi_adev_get_parameters,
        .get_input_buffer_size = hdmi_adev_get_input_buffer_size,
        .open_output_stream = hdmi_adev_open_output_stream,
        .close_output_stream = hdmi_adev_close_output_stream,
        .open_input_stream =  hdmi_adev_open_input_stream,
        .close_input_stream = hdmi_adev_close_input_stream,
        .dump = hdmi_adev_dump,
    },

    // Don't reorder channels until a valid CEA mapping has been
    // explicitly set. IOW, assume default channel mapping is
    // fine until some other mapping is requested.
    .CEAMap = true,
    .map = {0},
};

static int hdmi_adev_open(const hw_module_t* module,
                          const char* name,
                          hw_device_t** device)
{
    TRACE();

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    hdmi_adev.device.common.module = (struct hw_module_t *) module;
    *device = &hdmi_adev.device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = hdmi_adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "OMAP HDMI audio HW HAL",
        .author = "Texas Instruments",
        .methods = &hal_module_methods,
    },
};
