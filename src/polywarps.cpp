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
#include "warps/dsp/modulator.h"

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

#define WARPS_URI "http://polyeffects.com/lv2/polywarps#"

/**
   In code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability.
*/
typedef enum {
    ALGORITHM_PARAM = 0,
    TIMBRE_PARAM = 1,
    SHAPE_PARAM = 2,
    LEVEL1_PARAM = 3,
    LEVEL2_PARAM = 4,
    LEVEL1_INPUT = 5,
    LEVEL2_INPUT = 6,
    ALGORITHM_INPUT = 7,
    TIMBRE_INPUT = 8,
    CARRIER_INPUT = 9,
    MODULATOR_INPUT = 10,
    MODULATOR_OUTPUT = 11,
    AUX_OUTPUT = 12,
} PortIndex;

/**
   Every plugin defines a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method.  In this simple plugin, only port buffers need to be
   stored, since there is no additional instance data.
*/
typedef struct {
	// Port buffers
    const float* algorithm_param;
    const float* timbre_param;
    const float* shape_param;
    const float* level1_param;
    const float* level2_param;
    const float* level1_input;
    const float* level2_input;
    const float* algorithm_input;
    const float* timbre_input;
    const float* carrier_input;
    const float* modulator_input;
    float* modulator_output;
    float* aux_output;

	warps::Modulator modulator;

	float algorithm_smooth = 0.5f;
    float timbre_smooth = 0.5f;
    float level1_smooth = 0.5f;
    float level2_smooth = 0.5f;

} Warps;

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Warps* amp = new Warps();

	memset(&amp->modulator, 0, sizeof(amp->modulator));
    amp->modulator.Init(48000.0f);

	// set feature mode for the difference modules.
	if (!strcmp (descriptor->URI, WARPS_URI "doppler")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_DOPPLER);
	} else if (!strcmp (descriptor->URI, WARPS_URI "fold")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_FOLD);
	} else if (!strcmp (descriptor->URI, WARPS_URI "chebyschev")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_CHEBYSCHEV);
	} else if (!strcmp (descriptor->URI, WARPS_URI "frequency_shifter")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_FREQUENCY_SHIFTER);
	} else if (!strcmp (descriptor->URI, WARPS_URI "bitcrusher")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_BITCRUSHER);
	} else if (!strcmp (descriptor->URI, WARPS_URI "comparator")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_COMPARATOR);
	} else if (!strcmp (descriptor->URI, WARPS_URI "vocoder")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_VOCODER);
	} else if (!strcmp (descriptor->URI, WARPS_URI "delay")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_DELAY);
	} else if (!strcmp (descriptor->URI, WARPS_URI "meta")) {
		amp->modulator.set_feature_mode(warps::FEATURE_MODE_META);
	}

	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Warps* amp = (Warps*)instance;

    switch ((PortIndex)port) {
        case ALGORITHM_PARAM:
            amp->algorithm_param = (const float*)data;
            break;
        case TIMBRE_PARAM:
            amp->timbre_param = (const float*)data;
            break;
        case SHAPE_PARAM:
            amp->shape_param = (const float*)data;
            break;
        case LEVEL1_PARAM:
            amp->level1_param = (const float*)data;
            break;
        case LEVEL2_PARAM:
            amp->level2_param = (const float*)data;
            break;
        case LEVEL1_INPUT:
            amp->level1_input = (const float*)data;
            break;
        case LEVEL2_INPUT:
            amp->level2_input = (const float*)data;
            break;
        case ALGORITHM_INPUT:
            amp->algorithm_input = (const float*)data;
            break;
        case TIMBRE_INPUT:
            amp->timbre_input = (const float*)data;
            break;
        case CARRIER_INPUT:
            amp->carrier_input = (const float*)data;
            break;
        case MODULATOR_INPUT:
            amp->modulator_input = (const float*)data;
            break;
        case MODULATOR_OUTPUT:
            amp->modulator_output = (float*)data;
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
	Warps* const amp = (Warps*)instance;

    const float algorithm_param = *(amp->algorithm_param);
    const float timbre_param = *(amp->timbre_param);
	const uint8_t shape_param = (uint8_t)round(*(amp->shape_param));
    const float level1_param = *(amp->level1_param);
    const float level2_param = *(amp->level2_param);
    const float* level1_input = amp->level1_input;
    const float* level2_input = amp->level2_input;
    const float* algorithm_input = amp->algorithm_input;
    const float* timbre_input = amp->timbre_input;
    const float* carrier_input = amp->carrier_input;
    const float* modulator_input = amp->modulator_input;

    float* modulator_output = amp->modulator_output;
    float* aux_output = amp->aux_output;

	warps::Modulator *modulator = &(amp->modulator);

	uint32_t block_size = 32;
	if (n_samples < block_size){
		block_size = n_samples;	
	}

	uint32_t pos = 0;
	while (pos < n_samples){
		warps::ShortFrame input[block_size];
		for (uint32_t i = 0; i < block_size; i++) {
			input[i].l = clamp(carrier_input[pos+i] * 32767.0f, -32768.0f, 32767.0f);
			input[i].r = clamp(modulator_input[pos+i] * 32767.0f, -32768.0f, 32767.0f);
		}
		uint32_t j = pos;

        warps::Parameters *p = modulator->mutable_parameters();

        p->carrier_shape = shape_param % 4;

        amp->level1_smooth += .008f * (level1_param - amp->level1_smooth);
        amp->level2_smooth += .008f * (level2_param - amp->level2_smooth);
        amp->algorithm_smooth += .008f * (algorithm_param - amp->algorithm_smooth);
        amp->timbre_smooth += .008f * (timbre_param - amp->timbre_smooth);

        p->channel_drive[0] = clamp(amp->level1_smooth + level1_input[j], 0.0f, 1.0f);
        p->channel_drive[1] = clamp(amp->level2_smooth + level2_input[j], 0.0f, 1.0f);
        p->modulation_algorithm = clamp(amp->algorithm_smooth / 8.0f + algorithm_input[j], 0.0f, 1.0f);
        p->modulation_parameter = clamp(amp->timbre_smooth + timbre_input[j], 0.0f, 1.0f);

		p->raw_level[0] = clamp(amp->level1_smooth, 0.0f, 1.0f);
		p->raw_level[1] = clamp(amp->level2_smooth, 0.0f, 1.0f);

		p->raw_algorithm_pot = amp->algorithm_smooth / 8.0;
        p->raw_algorithm_cv  = clamp(algorithm_input[j], -1.0f, 1.0f);
        p->raw_algorithm = p->modulation_algorithm;

        p->note = 60.0 * amp->level1_smooth + 12.0 * level1_input[j]+ 12.0;

		warps::ShortFrame output[block_size];
        modulator->Process(input, output, block_size);

		for (uint32_t i = 0; i < block_size; i++) {
			modulator_output[pos+i] = output[i].l / 32768.0;
			aux_output[pos+i] = output[i].r / 32768.0;
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
	Warps* amp = (Warps*)instance;
	/* delete amp->modulator; */
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
	WARPS_URI "doppler",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor1 = {
	WARPS_URI "fold",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor2 = {
	WARPS_URI "chebyschev",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor3 = {
	WARPS_URI "frequency_shifter",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor4 = {
	WARPS_URI "bitcrusher",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor5 = {
	WARPS_URI "comparator",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor6 = {
	WARPS_URI "vocoder",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor7 = {
	WARPS_URI "delay",
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor8 = {
	WARPS_URI "meta",
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
		case 7:
			return &descriptor7;
		case 8:
			return &descriptor8;
		default:
			return NULL;
	}
}
