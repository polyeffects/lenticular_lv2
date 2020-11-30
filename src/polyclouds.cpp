/*
  Copyright 2020 Loki Davison <loki@polyeffects.com>
  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.
  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "lv2/core/lv2.h"

/** Include standard C headers */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "clouds/dsp/granular_processor.h"

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

#define CLOUDS_URI "http://polyeffects.com/lv2/polyclouds#"

/**
   In code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability.
*/
typedef enum {

	// CVs
    FREEZE_INPUT = 0,
    TRIG_INPUT = 1,
    POSITION_INPUT = 2,
    SIZE_INPUT = 3,
    PITCH_INPUT = 4,
    BLEND_INPUT = 5,
    SPREAD_INPUT = 6,
    FEEDBACK_INPUT = 7,
    REVERB_INPUT = 8,
    DENSITY_INPUT = 9,
    TEXTURE_INPUT = 10,
	REVERSE_INPUT = 11,
    IN_L_INPUT = 12,
    IN_R_INPUT = 13,
    OUT_L_OUTPUT = 14,
    OUT_R_OUTPUT = 15,
	// Controls
	POSITION_PARAM = 16,
    SIZE_PARAM = 17,
    PITCH_PARAM = 18,
    IN_GAIN_PARAM = 19,
    DENSITY_PARAM = 20,
    TEXTURE_PARAM = 21,
    BLEND_PARAM = 22,
    SPREAD_PARAM = 23,
    FEEDBACK_PARAM = 24,
    REVERB_PARAM = 25,
    FREEZE_PARAM = 26,
	REVERSE_PARAM = 27,
} PortIndex;

/**
   Every plugin defines a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method.  In this simple plugin, only port buffers need to be
   stored, since there is no additional instance data.
*/
typedef struct {
	// Port buffers
    const float* freeze;
    const float* trig;
    const float* position;
    const float* size;
    const float* pitch;
    const float* blend;
    const float* spread;
    const float* feedback;
    const float* reverb;
    const float* density;
    const float* texture;
	const float* reverse;
    const float* in_l;
    const float* in_r;
	//
    float* out_l;
    float* out_r;
	//
	const float* position_param;
    const float* size_param;
    const float* pitch_param;
    const float* in_gain_param;
    const float* density_param;
    const float* texture_param;
    const float* blend_param;
    const float* spread_param;
    const float* feedback_param;
    const float* reverb_param;
    const float* freeze_param;
    const float* reverse_param;
	/* bool is_on; */
	/* bool was_down; */
	int buffersize = 1;
	int currentbuffersize = 1;
	bool lofi = false;
	bool mono = false;
	uint8_t *block_mem;
	uint8_t *block_ccm;
	clouds::GranularProcessor *processor;
	float in_gain_smooth = 1.0f;
	float position_smooth = 0.5f;
    float size_smooth = 0.5f;
    float pitch_smooth = 0.5f;
    float density_smooth = 0.5f;
    float texture_smooth = 0.5f;
    float blend_smooth = 0.5f;
    float spread_smooth = 0.5f;
    float feedback_smooth = 0.5f;
    float reverb_smooth = 0.5f;

	bool prev_trigger = false;

} Clouds;

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Clouds* amp = (Clouds*)calloc(1, sizeof(Clouds));
	const int memLen = 118784;//*256;
	const int ccmLen = 65536 - 128;
	amp->block_mem = new uint8_t[memLen]();
	amp->block_ccm = new uint8_t[ccmLen]();
	amp->processor = new clouds::GranularProcessor();
	memset(amp->processor, 0, sizeof(*amp->processor));
	amp->processor->Init(amp->block_mem, memLen, amp->block_ccm, ccmLen);

	// set feature mode for the difference modules.
	if (!strcmp (descriptor->URI, CLOUDS_URI "granular")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
	} else if (!strcmp (descriptor->URI, CLOUDS_URI "stretch")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
	} else if (!strcmp (descriptor->URI, CLOUDS_URI "looping_delay")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_LOOPING_DELAY);
	} else if (!strcmp (descriptor->URI, CLOUDS_URI "spectral")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_SPECTRAL);
	} else if (!strcmp (descriptor->URI, CLOUDS_URI "oliverb")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_OLIVERB);
	} else if (!strcmp (descriptor->URI, CLOUDS_URI "resonestor")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_RESONESTOR);
	} else if (!strcmp (descriptor->URI, CLOUDS_URI "beat_repeat")) {
		amp->processor->set_playback_mode(clouds::PLAYBACK_MODE_KAMMERL);
	}
	
	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Clouds* amp = (Clouds*)instance;

	switch ((PortIndex)port) {
		case FREEZE_INPUT:
			amp->freeze = (const float*)data;
			break;
		case TRIG_INPUT:
			amp->trig = (const float*)data;
			break;
		case POSITION_INPUT:
			amp->position = (const float*)data;
			break;
		case SIZE_INPUT:
			amp->size = (const float*)data;
			break;
		case PITCH_INPUT:
			amp->pitch = (const float*)data;
			break;
		case BLEND_INPUT:
			amp->blend = (const float*)data;
			break;
		case SPREAD_INPUT:
			amp->spread = (const float*)data;
			break;
		case FEEDBACK_INPUT:
			amp->feedback = (const float*)data;
			break;
		case REVERB_INPUT:
			amp->reverb = (const float*)data;
			break;
		case IN_L_INPUT:
			amp->in_l = (const float*)data;
			break;
		case IN_R_INPUT:
			amp->in_r = (const float*)data;
			break;
		case DENSITY_INPUT:
			amp->density = (const float*)data;
			break;
		case TEXTURE_INPUT:
			amp->texture = (const float*)data;
			break;
		case REVERSE_INPUT:
			amp->reverse = (const float*)data;
			break;
		case OUT_L_OUTPUT:
			amp->out_l = (float*)data;
			break;
		case OUT_R_OUTPUT:
			amp->out_r = (float*)data;
			break;
		case POSITION_PARAM:
			amp->position_param = (const float*)data;
			break;
		case SIZE_PARAM:
			amp->size_param = (const float*)data;
			break;
		case PITCH_PARAM:
			amp->pitch_param = (const float*)data;
			break;
		case IN_GAIN_PARAM:
			amp->in_gain_param = (const float*)data;
			break;
		case DENSITY_PARAM:
			amp->density_param = (const float*)data;
			break;
		case TEXTURE_PARAM:
			amp->texture_param = (const float*)data;
			break;
		case BLEND_PARAM:
			amp->blend_param = (const float*)data;
			break;
		case SPREAD_PARAM:
			amp->spread_param = (const float*)data;
			break;
		case FEEDBACK_PARAM:
			amp->feedback_param = (const float*)data;
			break;
		case REVERB_PARAM:
			amp->reverb_param = (const float*)data;
			break;
		case FREEZE_PARAM:
			amp->freeze_param = (const float*)data;
			break;
		case REVERSE_PARAM:
			amp->reverse_param = (const float*)data;
			break;

		default:
			break;
	}
}

/**
   The `activate()` method is called by the host to initialise and prepare the
   plugin instance for running.  The plugin must reset all internal state
   except for buffer locations set by `connect_port()`.  Since this plugin has
   no other internal state, this method does nothing.
   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
activate(LV2_Handle instance)
{
}


/**
   The `run()` method is the main process function of the plugin.  It processes
   a block of audio in the audio context.  Since this plugin is
   `lv2:hardRTCapable`, `run()` must be real-time safe, so blocking (e.g. with
   a mutex) or memory allocation are not allowed.
*/
static void
run(LV2_Handle instance, uint32_t n_samples)
{
	Clouds* const amp = (Clouds*)instance;

    const float* freeze = amp->freeze;
    const float* trig = amp->trig;
    const float* position = amp->position;
    const float* size = amp->size;
    const float* pitch = amp->pitch;
    const float* blend = amp->blend;
    const float* spread = amp->spread;
    const float* feedback = amp->feedback;
    const float* reverb = amp->reverb;
    const float* in_l = amp->in_l;
    const float* in_r = amp->in_r;
    const float* density = amp->density;
    const float* texture = amp->texture;
	const float* reverse = amp->reverse;
	//
    float* out_l = amp->out_l;
    float* out_r = amp->out_r;


	const float position_param = *(amp->position_param);
    const float size_param = *(amp->size_param);
    const float pitch_param = *(amp->pitch_param);
    const float in_gain_param = *(amp->in_gain_param);
    const float density_param = *(amp->density_param);
    const float texture_param = *(amp->texture_param);
    const float blend_param = *(amp->blend_param);
    const float spread_param = *(amp->spread_param);
    const float feedback_param = *(amp->feedback_param);
    const float reverb_param = *(amp->reverb_param);
    const float freeze_param = *(amp->freeze_param);
    const float reverse_param = *(amp->reverse_param);
	//
	clouds::GranularProcessor *processor = amp->processor;

	// Trigger Do i need to persist? 
	bool triggered = false;
	uint32_t block_size = 32;
	if (n_samples < block_size){
		block_size = n_samples;	
	}
	uint32_t pos = 0;
	while (pos < n_samples){
		triggered = false;
		if (trig[pos] >= 1) {
			if (!(amp->prev_trigger)){
				triggered = true;
				amp->prev_trigger = true;
			}
		} else {
			amp->prev_trigger = false;
		}

		clouds::ShortFrame input[block_size];
		for (uint32_t i = 0; i < block_size; i++) {
			amp->in_gain_smooth += .008f * (in_gain_param - amp->in_gain_smooth);
			/* input[i].l = clamp(in_l[pos+i] * amp->in_gain_smooth * 32767.0f, -32768.0f, 32767.0f); */
			/* input[i].r = clamp(in_r[pos+i] * amp->in_gain_smooth * 32767.0f, -32768.0f, 32767.0f); */
			input[i].l = clamp(in_l[pos+i] * 32767.0f, -32768.0f, 32767.0f);
			input[i].r = clamp(in_r[pos+i] * 32767.0f, -32768.0f, 32767.0f);
		}
		// Set up processor
		/* processor->set_num_channels(amp->mono ? 1 : 2); */
		/* processor->set_low_fidelity(amp->lofi); */
		processor->set_num_channels(2);
		processor->set_low_fidelity(false);
		processor->Prepare();

		uint32_t j = pos;


		clouds::Parameters* p = processor->mutable_parameters();
		p->trigger = triggered;
		p->gate = triggered;
		p->freeze = (freeze[j] >= 1.0 || freeze_param >= 1.0f);
		p->granular.reverse = (reverse_param >= 1.0f || reverse[j] >= 1.0f);

        amp->position_smooth += .008f * (position_param - amp->position_smooth);
		p->position = clamp(amp->position_smooth + position[j], 0.0f, 1.0f);
        amp->size_smooth += .008f * (size_param - amp->size_smooth);
		p->size = clamp(amp->size_smooth + size[j], 0.0f, 1.0f);
        amp->pitch_smooth += .008f * (pitch_param - amp->pitch_smooth);
		p->pitch = clamp(amp->pitch_smooth + (pitch[j] * 12.0f), -48.0f, 48.0f);
        amp->density_smooth += .008f * (density_param - amp->density_smooth);
		p->density = clamp(amp->density_smooth + density[j], 0.0f, 1.0f);
        amp->texture_smooth += .008f * (texture_param - amp->texture_smooth);
		p->texture = clamp(amp->texture_smooth + texture[j], 0.0f, 1.0f);
        amp->blend_smooth += .008f * (blend_param - amp->blend_smooth);
		p->dry_wet = clamp(amp->blend_smooth + blend[j], 0.0f, 1.0f);
        amp->spread_smooth += .008f * (spread_param - amp->spread_smooth);
		p->stereo_spread =  clamp(amp->spread_smooth + spread[j], 0.0f, 1.0f);
        amp->feedback_smooth += .008f * (feedback_param - amp->feedback_smooth);
		p->feedback =  clamp(amp->feedback_smooth + feedback[j], 0.0f, 1.0f);
        amp->reverb_smooth += .008f * (reverb_param - amp->reverb_smooth);
		p->reverb =  clamp(amp->reverb_smooth + reverb[j], 0.0f, 1.0f);

		clouds::ShortFrame output[block_size];
		processor->Process(input, output, block_size);

		for (uint32_t i = 0; i < block_size; i++) {
			out_l[pos+i] = output[i].l / 32768.0;
			out_r[pos+i] = output[i].r / 32768.0;
		}
		pos = pos + block_size;
		if (pos+block_size > n_samples){
			block_size = n_samples - pos;
		}
	}

}


/**
   The `deactivate()` method is the counterpart to `activate()`, and is called by
   the host after running the plugin.  It indicates that the host will not call
   `run()` again until another call to `activate()` and is mainly useful for more
   advanced plugins with ``live'' characteristics such as those with auxiliary
   processing threads.  As with `activate()`, this plugin has no use for this
   information so this method does nothing.
   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
deactivate(LV2_Handle instance)
{
}

/**
   Destroy a plugin instance (counterpart to `instantiate()`).
   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
cleanup(LV2_Handle instance)
{
	Clouds* const amp = (Clouds*)instance;
	delete amp->processor;
	delete[] amp->block_mem;
	delete[] amp->block_ccm;
	free(instance);
}

/**
   The `extension_data()` function returns any extension data supported by the
   plugin.  Note that this is not an instance method, but a function on the
   plugin descriptor.  It is usually used by plugins to implement additional
   interfaces.  This plugin does not have any extension data, so this function
   returns NULL.
   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
static const void*
extension_data(const char* uri)
{
	return NULL;
}

/**
   Every plugin must define an `LV2_Descriptor`.  It is best to define
   descriptors statically to avoid leaking memory and non-portable shared
   library constructors and destructors to clean up properly.
*/
static const LV2_Descriptor descriptor0 = {
	CLOUDS_URI "granular",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor1 = {
	CLOUDS_URI "stretch",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor2 = {
	CLOUDS_URI "looping_delay",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor3 = {
	CLOUDS_URI "spectral",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor4 = {
	CLOUDS_URI "oliverb",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor5 = {
	CLOUDS_URI "resonestor",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor6 = {
	CLOUDS_URI "beat_repeat",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

/**
   The `lv2_descriptor()` function is the entry point to the plugin library.  The
   host will load the library and call this function repeatedly with increasing
   indices to find all the plugins defined in the library.  The index is not an
   indentifier, the URI of the returned descriptor is used to determine the
   identify of the plugin.
   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
		case 0:
			return &descriptor0;
		case 1:
			return &descriptor1;
		case 2:
			return &descriptor2;
		case 3:
			return &descriptor3;
		case 4:
			return &descriptor4;
		case 5:
			return &descriptor5;
		case 6:
			return &descriptor6;
		default:
			return NULL;
	}
}
