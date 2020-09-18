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

#include "plaits/dsp/voice.h"
#include "lv2/core/lv2.h"

/** Include standard C headers */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

#define PLAITS_URI "http://polyeffects.com/lv2/polyplaits"

/**
   In code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability.
*/
typedef enum {
		MODEL_PARAM = 0,
		FREQ_PARAM,
		HARMONICS_PARAM,
		TIMBRE_PARAM,
		MORPH_PARAM,
		TIMBRE_CV_PARAM,
		FREQ_CV_PARAM,
		MORPH_CV_PARAM,
		LPG_COLOR_PARAM,
		LPG_DECAY_PARAM,
		// CV
		ENGINE_INPUT,
		TIMBRE_INPUT,
		FREQ_INPUT, // FM
		MORPH_INPUT,
		HARMONICS_INPUT,
		TRIGGER_INPUT,
		LEVEL_INPUT,
		NOTE_INPUT, // V/OCT
		//  OUT
		OUT_OUTPUT,
		AUX_OUTPUT,
} PortIndex;

/**
   Every plugin defines a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method.  In this simple plugin, only port buffers need to be
   stored, since there is no additional instance data.
*/
typedef struct {
	const float* model_param;
	const float* freq_param;
	const float* harmonics_param;
	const float* timbre_param;
	const float* morph_param;
	const float* timbre_cv_param;
	const float* freq_cv_param;
	const float* morph_cv_param;
	const float* lpg_color_param;
	const float* lpg_decay_param;
	// cv
	const float* engine_input;
	const float* timbre_input;
	const float* freq_input;
	const float* morph_input;
	const float* harmonics_input;
	const float* trigger_input;
	const float* level_input;
	const float* note_input;
	//  out
	float* out_output;
	float* aux_output;

	float algorithm_smooth = 0.5f;
    float timbre_smooth = 0.5f;
    float level1_smooth = 0.5f;
    float level2_smooth = 0.5f;
	/* uint8_t *shared_buf; */
	bool lowCpu = false;

	/* plaits::Voice voice = {}; */
	/* plaits::Patch patch = {}; */
	plaits::Voice* voice;
	plaits::Patch*  patch;
	/* char shared_buffer[16][16384] = {}; */
	char shared_buffer[16384] = {};
	/* char* shared_buffer; */

} Plaits;


static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	/* Plaits* amp = (Plaits*)malloc(sizeof(Plaits)); */
	Plaits* amp = new Plaits();
	/* for (int i = 0; i < 16; i++) { */
	/* const int memLen = 16384; */
	/* amp->shared_buf = new uint8_t[memLen](); */
	amp->voice = new plaits::Voice();
	amp->patch = new plaits::Patch();
	stmlib::BufferAllocator allocator(amp->shared_buffer, 16384);
	/* stmlib::BufferAllocator allocator(amp->shared_buf, memLen); */
	amp->voice->Init(&allocator);
	/* } */

	amp->patch->engine = 0;
	amp->patch->lpg_colour = 0.5f;
	amp->patch->decay = 0.5f;

	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Plaits* amp = (Plaits*)instance;

    switch ((PortIndex)port) {
		case MODEL_PARAM:
			amp->model_param = (const float*)data;
			break;
		case FREQ_PARAM:
			amp->freq_param = (const float*)data;
			break;
		case HARMONICS_PARAM:
			amp->harmonics_param = (const float*)data;
			break;
		case TIMBRE_PARAM:
			amp->timbre_param = (const float*)data;
			break;
		case MORPH_PARAM:
			amp->morph_param = (const float*)data;
			break;
		case TIMBRE_CV_PARAM:
			amp->timbre_cv_param = (const float*)data;
			break;
		case FREQ_CV_PARAM:
			amp->freq_cv_param = (const float*)data;
			break;
		case MORPH_CV_PARAM:
			amp->morph_cv_param = (const float*)data;
			break;
		case LPG_COLOR_PARAM:
			amp->lpg_color_param = (const float*)data;
			break;
		case LPG_DECAY_PARAM:
			amp->lpg_decay_param = (const float*)data;
			break;
			// CV
		case ENGINE_INPUT:
			amp->engine_input = (const float*)data;
			break;
		case TIMBRE_INPUT:
			amp->timbre_input = (const float*)data;
			break;
		case FREQ_INPUT:
			amp->freq_input = (const float*)data;
			break;
		case MORPH_INPUT:
			amp->morph_input = (const float*)data;
			break;
		case HARMONICS_INPUT:
			amp->harmonics_input = (const float*)data;
			break;
		case TRIGGER_INPUT:
			amp->trigger_input = (const float*)data;
			break;
		case LEVEL_INPUT:
			amp->level_input = (const float*)data;
			break;
		case NOTE_INPUT:
			amp->note_input = (const float*)data;
			break;
			//  OUT
		case OUT_OUTPUT:
			amp->out_output = (float*)data;
			break;
		case AUX_OUTPUT:
			amp->aux_output = (float*)data;
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
	Plaits* const amp = (Plaits*)instance;
	const uint8_t model_param = (uint8_t)round(*(amp->model_param)) % 16;
	//const int blockSize = 12;

	/* const float model_param = *(amp->model_param); */
	const float freq_param = *(amp->freq_param);
	const float harmonics_param = *(amp->harmonics_param);
	const float timbre_param = *(amp->timbre_param);
	const float morph_param = *(amp->morph_param);
	const float timbre_cv_param = *(amp->timbre_cv_param);
	const float freq_cv_param = *(amp->freq_cv_param);
	const float morph_cv_param = *(amp->morph_cv_param);
	const float lpg_color_param = *(amp->lpg_color_param);
	const float lpg_decay_param = *(amp->lpg_decay_param);
	// cv
	const float* engine_input = amp->engine_input;
	const float* timbre_input = amp->timbre_input;
	const float* freq_input = amp->freq_input;
	const float* morph_input = amp->morph_input;
	const float* harmonics_input = amp->harmonics_input;
	const float* trigger_input = amp->trigger_input;
	const float* level_input = amp->level_input;
	const float* note_input = amp->note_input;
	//  out
	float* out_output = amp->out_output;
	float* aux_output = amp->aux_output;
	

	amp->patch->engine = model_param;
	// Update patch
	amp->patch->note = 60.f + freq_param * 12.f;
	amp->patch->harmonics = harmonics_param;
	amp->patch->timbre = timbre_param;
	amp->patch->morph = morph_param;
	amp->patch->lpg_colour = lpg_color_param;
	amp->patch->decay = lpg_decay_param;
	amp->patch->frequency_modulation_amount = freq_cv_param;
	amp->patch->timbre_modulation_amount = timbre_cv_param;
	amp->patch->morph_modulation_amount = morph_cv_param;

	uint32_t block_size = 12;
	if (n_samples < block_size){
		block_size = n_samples;	
	}

	const bool freq_patched = freq_input[0] > -40.0f;
	const bool timbre_patched = timbre_input[0] > -40.0f;
	const bool morph_patched = morph_input[0] > -40.0f;
	const bool trigger_patched = trigger_input[0] > -40.0f;
	const bool level_patched = level_input[0] > -40.0f;

	uint32_t pos = 0;
	while (pos < n_samples){

		uint32_t s = pos;
		// Construct modulations
		plaits::Modulations modulations;
		modulations.engine = engine_input[s];
		modulations.note = note_input[s] * 12.f;
		modulations.harmonics = harmonics_input[s];
		// Triggers at around 0.7 V
		if (trigger_patched) {
			modulations.trigger = trigger_input[s] * 3.3f;
		} else {
			modulations.trigger = 0;
		}
		if (level_patched) {
			modulations.level = level_input[s] * 0.625; 
		} else {
			modulations.level = 0;
		}
		if (freq_patched) {
			modulations.frequency = freq_input[s] * 60.f;
		} else {
			modulations.frequency = 0;
		}
		if (timbre_patched) {
			modulations.timbre = timbre_input[s] * 0.625;
		} else {
			modulations.timbre = 0;
		}
		if (morph_patched) {
			modulations.morph = morph_input[s] * 0.625;
		} else {
			modulations.morph = 0;
		}

		modulations.frequency_patched = freq_patched; 
		modulations.timbre_patched = timbre_patched; 
		modulations.morph_patched = morph_patched;
		modulations.trigger_patched = trigger_patched;
		modulations.level_patched = level_patched;

		// Render frames
		plaits::Voice::Frame output[block_size];
		amp->voice->Render(*(amp->patch), modulations, output, block_size);

		// Convert output to frames
		for (uint32_t i = 0; i < block_size; i++) {
			out_output[pos+i] = output[i].out / 32768.f;
			aux_output[pos+i] = output[i].aux / 32768.f;
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
	Plaits* amp = (Plaits*)instance;
	/* delete amp->modulator; */
	delete amp->voice;
	delete amp->patch;
	delete amp;
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
	PLAITS_URI,
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
lv2_descriptor (uint32_t index)
{
	switch (index) {
		case 0:
			return &descriptor0;
		default:
			return NULL;
	}
}
