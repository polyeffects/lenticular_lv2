#include "lv2/core/lv2.h"

/** Include standard C headers */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "rings/dsp/part.h"
#include "rings/dsp/strummer.h"
#include "rings/dsp/string_synth_part.h"

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

#define RINGS_URI "http://polyeffects.com/lv2/polyrings"


typedef enum {
// control rate params
		POLYPHONY_PARAM,
		RESONATOR_PARAM,
		FREQUENCY_PARAM,
		STRUCTURE_PARAM,
		BRIGHTNESS_PARAM,
		DAMPING_PARAM,
		POSITION_PARAM,
		BRIGHTNESS_MOD_PARAM,
		FREQUENCY_MOD_PARAM,
		DAMPING_MOD_PARAM,
		STRUCTURE_MOD_PARAM,
		POSITION_MOD_PARAM,
// CV inputs
		BRIGHTNESS_MOD_INPUT,
		FREQUENCY_MOD_INPUT,
		DAMPING_MOD_INPUT,
		STRUCTURE_MOD_INPUT,
		POSITION_MOD_INPUT,
		STRUM_INPUT,
		PITCH_INPUT,
		IN_INPUT,
        // outputs
		ODD_OUTPUT,
		EVEN_OUTPUT,
		INTERNAL_EXCITER_PARAM,
} PortIndex;

struct Rings {

	uint16_t reverb_buffer[32768] = {};
	rings::Part part;
	rings::StringSynthPart string_synth;
	rings::Strummer strummer;
	bool strum = false;
	bool lastStrum = false;

	bool easterEgg = false;

    // port buffers
    // control rate params
    const float* polyphony_param;
    const float* resonator_param;
    const float* frequency_param;
    const float* structure_param;
    const float* brightness_param;
    const float* damping_param;
    const float* position_param;
    const float* brightness_mod_param;
    const float* frequency_mod_param;
    const float* damping_mod_param;
    const float* structure_mod_param;
    const float* position_mod_param;
    // CV inputs
    const float* brightness_mod_input;
    const float* frequency_mod_input;
    const float* damping_mod_input;
    const float* structure_mod_input;
    const float* position_mod_input;
    const float* strum_input;
    const float* pitch_input;
    const float* in_input;
    // outputs
    float* odd_output;
    float* even_output;
    const float* internal_exciter_param;

	Rings(double rate) {
		/* config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS); */
		/* configParam(POLYPHONY_PARAM, 0.0, 1.0, 0.0, "Polyphony"); */
		/* configParam(RESONATOR_PARAM, 0.0, 1.0, 0.0, "Resonator type"); */
		/* configParam(FREQUENCY_PARAM, 0.0, 60.0, 30.0, "Coarse frequency adjustment"); */
		/* configParam(STRUCTURE_PARAM, 0.0, 1.0, 0.5, "Harmonic structure"); */
		/* configParam(BRIGHTNESS_PARAM, 0.0, 1.0, 0.5, "Brightness"); */
		/* configParam(DAMPING_PARAM, 0.0, 1.0, 0.5, "Decay time"); */
		/* configParam(POSITION_PARAM, 0.0, 1.0, 0.5, "Excitation position"); */
		/* configParam(BRIGHTNESS_MOD_PARAM, -1.0, 1.0, 0.0, "Brightness attenuverter"); */
		/* configParam(FREQUENCY_MOD_PARAM, -1.0, 1.0, 0.0, "Frequency attenuverter"); */
		/* configParam(DAMPING_MOD_PARAM, -1.0, 1.0, 0.0, "Damping attenuverter"); */
		/* configParam(STRUCTURE_MOD_PARAM, -1.0, 1.0, 0.0, "Structure attenuverter"); */
		/* configParam(POSITION_MOD_PARAM, -1.0, 1.0, 0.0, "Position attenuverter"); */

		strummer.Init(0.01, rate / 24);
		part.Init(reverb_buffer);
		string_synth.Init(reverb_buffer);
	}
};

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Rings* amp = (Rings*) new Rings(rate);


	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Rings* amp = (Rings*)instance;

	switch ((PortIndex)port) {

        case POLYPHONY_PARAM:
            amp->polyphony_param = (const float*)data;
            break;
        case RESONATOR_PARAM:
            amp->resonator_param = (const float*)data;
            break;
        case FREQUENCY_PARAM:
            amp->frequency_param = (const float*)data;
            break;
        case STRUCTURE_PARAM:
            amp->structure_param = (const float*)data;
            break;
        case BRIGHTNESS_PARAM:
            amp->brightness_param = (const float*)data;
            break;
        case DAMPING_PARAM:
            amp->damping_param = (const float*)data;
            break;
        case POSITION_PARAM:
            amp->position_param = (const float*)data;
            break;
        case BRIGHTNESS_MOD_PARAM:
            amp->brightness_mod_param = (const float*)data;
            break;
        case FREQUENCY_MOD_PARAM:
            amp->frequency_mod_param = (const float*)data;
            break;
        case DAMPING_MOD_PARAM:
            amp->damping_mod_param = (const float*)data;
            break;
        case STRUCTURE_MOD_PARAM:
            amp->structure_mod_param = (const float*)data;
            break;
        case POSITION_MOD_PARAM:
            amp->position_mod_param = (const float*)data;
            break;
// CV inputs
        case BRIGHTNESS_MOD_INPUT:
            amp->brightness_mod_input = (const float*)data;
            break;
        case FREQUENCY_MOD_INPUT:
            amp->frequency_mod_input = (const float*)data;
            break;
        case DAMPING_MOD_INPUT:
            amp->damping_mod_input = (const float*)data;
            break;
        case STRUCTURE_MOD_INPUT:
            amp->structure_mod_input = (const float*)data;
            break;
        case POSITION_MOD_INPUT:
            amp->position_mod_input = (const float*)data;
            break;
        case STRUM_INPUT:
            amp->strum_input = (const float*)data;
            break;
        case PITCH_INPUT:
            amp->pitch_input = (const float*)data;
            break;
        case IN_INPUT:
            amp->in_input = (const float*)data;
            break;
        // outputs
        case ODD_OUTPUT:
            amp->odd_output = (float*)data;
            break;
        case EVEN_OUTPUT:
            amp->even_output = (float*)data;
            break;
        case INTERNAL_EXCITER_PARAM:
            amp->internal_exciter_param = (const float*)data;
            break;
		default:
			break;
	}
}

static void
activate(LV2_Handle instance)
{
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	Rings* const amp = (Rings*)instance;

    const float polyphony_param = *(amp->polyphony_param);
    const float resonator_param = *(amp->resonator_param);
    const float frequency_param = *(amp->frequency_param);
    const float structure_param = *(amp->structure_param);
    const float brightness_param = *(amp->brightness_param);
    const float damping_param = *(amp->damping_param);
    const float position_param = *(amp->position_param);
    const float brightness_mod_param = *(amp->brightness_mod_param);
    const float frequency_mod_param = *(amp->frequency_mod_param);
    const float damping_mod_param = *(amp->damping_mod_param);
    const float structure_mod_param = *(amp->structure_mod_param);
    const float position_mod_param = *(amp->position_mod_param);
    // CV inputs
    const float* brightness_mod_input = amp->brightness_mod_input;
    const float* frequency_mod_input = amp->frequency_mod_input;
    const float* damping_mod_input = amp->damping_mod_input;
    const float* structure_mod_input = amp->structure_mod_input;
    const float* position_mod_input = amp->position_mod_input;
    const float* strum_input = amp->strum_input;
    const float* pitch_input = amp->pitch_input;
    const float* in_input = amp->in_input;
    // outputs
    float* odd_output = amp->odd_output;
    float* even_output = amp->even_output;
    const float internal_exciter_param = *(amp->internal_exciter_param);

	int polyphonyMode = 0;
	rings::ResonatorModel resonatorModel = rings::RESONATOR_MODEL_MODAL;

    polyphonyMode = (int) (polyphony_param) % 3;
    resonatorModel = (rings::ResonatorModel)((int) (resonator_param) % 3);

	uint32_t block_size = 24;
	if (n_samples < block_size){
		block_size = n_samples;	
	}
	uint32_t pos = 0;

	bool strum = amp->strum;
	bool lastStrum = amp->lastStrum;

	while (pos < n_samples){
        uint32_t s = pos;


        // TODO
        // "Normalized to a pulse/burst generator that reacts to note changes on the V/OCT input."
        // Get input

        float in[24] = {};
        bool sub_strum = false;
		for (uint32_t i = 0; i < block_size; i++) {
            in[i] = clamp(in_input[s+i], 0.0f, 1.0f);
            if (strum_input[s+i] >= 1.0){
                sub_strum = true;
            
            }
        }
        lastStrum = strum;
        strum = sub_strum;

        // Polyphony
        int polyphony = 1 << polyphonyMode;
        if (amp->part.polyphony() != polyphony)
            amp->part.set_polyphony(polyphony);
        // Model
        if (amp->easterEgg)
            amp->string_synth.set_fx((rings::FxType) resonatorModel);
        else
            amp->part.set_model(resonatorModel);

        // Patch
        rings::Patch patch;
        float structure = structure_param + (structure_mod_param * structure_mod_input[s]);
        patch.structure = clamp(structure, 0.0f, 0.9995f);
        patch.brightness = clamp(brightness_param + (brightness_mod_param * brightness_mod_input[s]), 0.0f, 1.0f);
        patch.damping = clamp(damping_param + (damping_mod_param * damping_mod_input[s]), 0.0f, 0.9995f);
        patch.position = clamp(position_param + (position_mod_param * position_mod_input[s]), 0.0f, 0.9995f);

        // Performance
        rings::PerformanceState performance_state; 
		if  (pitch_input[s] > -40.0f){
            performance_state.note = 12.0 * pitch_input[s];
        } else {
            performance_state.note = 1 / 12.0;
        }
        float transpose = frequency_param;
        // Quantize transpose if pitch input is connected
        if (pitch_input[s] > -40.0f) {
            transpose = roundf(transpose);
        }
        performance_state.tonic = 12.0 + clamp(transpose, 0.0f, 60.0f);
        // default frequency_mod_input must be 1.
        performance_state.fm = clamp(48.0 * frequency_mod_param * frequency_mod_input[s], -48.0f, 48.0f);

        performance_state.internal_exciter = internal_exciter_param >= 1.0f;
        performance_state.internal_strum = !(strum_input[s] > -40.0f);
        performance_state.internal_note = !(pitch_input[s] > -40.0f);

        // TODO
        // "Normalized to a step detector on the V/OCT input and a transient detector on the IN input."
        performance_state.strum = strum && !lastStrum;

        performance_state.chord = clamp((int) roundf(structure * (rings::kNumChords - 1)), 0, rings::kNumChords - 1);

        // Process audio
        float out[24];
        float aux[24];
        if (amp->easterEgg) {
            amp->strummer.Process(NULL, block_size, &performance_state);
            amp->string_synth.Process(performance_state, patch, in, out, aux, block_size);
        }
        else {
            amp->strummer.Process(in, block_size, &performance_state);
            amp->part.Process(performance_state, patch, in, out, aux, block_size);
        }

		for (uint32_t i = 0; i < block_size; i++) {
            odd_output[s+i] = out[i];
            even_output[s+i] = aux[i];
        }

		pos = pos + block_size;
		if (pos+block_size > n_samples){
			block_size = n_samples - pos;
		}
    }

    amp->strum = strum;
    amp->lastStrum = lastStrum;

}




		/* menu->addChild(construct<RingsModelItem>(&MenuItem::text, "Modal resonator", &RingsModelItem::rings, rings, &RingsModelItem::model, rings::RESONATOR_MODEL_MODAL)); */
		/* menu->addChild(construct<RingsModelItem>(&MenuItem::text, "Sympathetic strings", &RingsModelItem::rings, rings, &RingsModelItem::model, rings::RESONATOR_MODEL_SYMPATHETIC_STRING)); */
		/* menu->addChild(construct<RingsModelItem>(&MenuItem::text, "Modulated/inharmonic string", &RingsModelItem::rings, rings, &RingsModelItem::model, rings::RESONATOR_MODEL_STRING)); */
		/* menu->addChild(construct<RingsModelItem>(&MenuItem::text, "FM voice", &RingsModelItem::rings, rings, &RingsModelItem::model, rings::RESONATOR_MODEL_FM_VOICE)); */
		/* menu->addChild(construct<RingsModelItem>(&MenuItem::text, "Quantized sympathetic strings", &RingsModelItem::rings, rings, &RingsModelItem::model, rings::RESONATOR_MODEL_SYMPATHETIC_STRING_QUANTIZED)); */
		/* menu->addChild(construct<RingsModelItem>(&MenuItem::text, "Reverb string", &RingsModelItem::rings, rings, &RingsModelItem::model, rings::RESONATOR_MODEL_STRING_AND_REVERB)); */
		/* menu->addChild(construct<RingsEasterEggItem>(&MenuItem::text, "Disastrous Peace", &RingsEasterEggItem::rings, rings)); */

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
	Rings* const amp = (Rings*)instance;
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
	RINGS_URI,
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
		default:
			return NULL;
	}
}

