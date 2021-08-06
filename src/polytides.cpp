#include "lv2/core/lv2.h"

/** Include standard C headers */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "stmlib/dsp/hysteresis_quantizer.h"
#include "stmlib/dsp/units.h"
#include "tides2/poly_slope_generator.h"
#include "tides2/ramp_extractor.h"
#include "tides2/io_buffer.h"

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

#define TIDES_URI "http://polyeffects.com/lv2/polytides"



static const float kRootScaled[3] = {
	0.125f,
	2.0f,
	130.81f
};

static const tides::Ratio kRatios[20] = {
	{ 0.0625f, 16 },
	{ 0.125f, 8 },
	{ 0.1666666f, 6 },
	{ 0.25f, 4 },
	{ 0.3333333f, 3 },
	{ 0.5f, 2 },
	{ 0.6666666f, 3 },
	{ 0.75f, 4 },
	{ 0.8f, 5 },
	{ 1, 1 },
	{ 1, 1 },
	{ 1.25f, 4 },
	{ 1.3333333f, 3 },
	{ 1.5f, 2 },
	{ 2.0f, 1 },
	{ 3.0f, 1 },
	{ 4.0f, 1 },
	{ 6.0f, 1 },
	{ 8.0f, 1 },
	{ 16.0f, 1 },
};


typedef enum {

// control rate params
		RANGE_PARAM,
		MODE_PARAM,
		RAMP_PARAM,
		FREQUENCY_PARAM,
		SHAPE_PARAM,
		SMOOTHNESS_PARAM,
		SLOPE_PARAM,
		SHIFT_PARAM,
		SLOPE_CV_PARAM,
		FREQUENCY_CV_PARAM,
		SMOOTHNESS_CV_PARAM,
		SHAPE_CV_PARAM,
		SHIFT_CV_PARAM,
// CV inputs
		SLOPE_INPUT,
		FREQUENCY_INPUT,
		V_OCT_INPUT,
		SMOOTHNESS_INPUT,
		SHAPE_INPUT,
		SHIFT_INPUT,
		TRIG_INPUT,
		CLOCK_INPUT,
// outs
		OUT1,
		OUT2,
		OUT3,
		OUT4,
		TEMPO_OUT,
} PortIndex;

struct Tides {

	tides::PolySlopeGenerator poly_slope_generator;
	tides::RampExtractor ramp_extractor;
	stmlib::HysteresisQuantizer ratio_index_quantizer;


	// Buffers
	stmlib::GateFlags trig_flags[tides::kBlockSize] = {};
	stmlib::GateFlags clock_flags[tides::kBlockSize] = {};
	stmlib::GateFlags previous_trig_flag = stmlib::GATE_FLAG_LOW;
	stmlib::GateFlags previous_clock_flag = stmlib::GATE_FLAG_LOW;

	bool must_reset_ramp_extractor = true;
	tides::OutputMode previous_output_mode = tides::OUTPUT_MODE_GATES;
	uint8_t frame = 0;
	double sample_rate = 48000.0;


    // port buffers
    // control rate params
	const float* range_param;
	const float* mode_param;
	const float* ramp_param;
	const float* frequency_param;
	const float* shape_param;
	const float* smoothness_param;
	const float* slope_param;
	const float* shift_param;
	const float* slope_cv_param;
	const float* frequency_cv_param;
	const float* smoothness_cv_param;
	const float* shape_cv_param;
	const float* shift_cv_param;
// CV inputs
	const float* slope_input;
	const float* frequency_input;
	const float* v_oct_input;
	const float* smoothness_input;
	const float* shape_input;
	const float* shift_input;
	const float* trig_input;
	const float* clock_input;
// outs
	float* out1;
	float* out2;
	float* out3;
	float* out4;
	float* tempo_out;

	Tides(double rate) {
		/* configParam(RANGE_PARAM, 0.0, 1.0, 0.0, "Frequency range"); */
		/* configParam(MODE_PARAM, 0.0, 1.0, 0.0, "Output mode"); */
		/* configParam(FREQUENCY_PARAM, -48, 48, 0.0, "Frequency"); */
		/* configParam(SHAPE_PARAM, 0.0, 1.0, 0.5, "Shape"); */
		/* configParam(RAMP_PARAM, 0.0, 1.0, 0.0, "Ramp mode"); */
		/* configParam(SMOOTHNESS_PARAM, 0.0, 1.0, 0.5, "Waveshape transformation"); */
		/* configParam(SLOPE_PARAM, 0.0, 1.0, 0.5, "Ascending/descending ratio"); */
		/* configParam(SHIFT_PARAM, 0.0, 1.0, 0.5, "Output polarization and shifting"); */
		/* configParam(SLOPE_CV_PARAM, -1.0, 1.0, 0.0, "Slope CV"); */
		/* configParam(FREQUENCY_CV_PARAM, -1.0, 1.0, 0.0, "Frequency CV"); */
		/* configParam(SMOOTHNESS_CV_PARAM, -1.0, 1.0, 0.0, "Smoothness CV"); */
		/* configParam(SHAPE_CV_PARAM, -1.0, 1.0, 0.0, "Shape CV"); */
		/* configParam(SHIFT_CV_PARAM, -1.0, 1.0, 0.0, "Shift CV"); */

		poly_slope_generator.Init();
		ratio_index_quantizer.Init();
		ramp_extractor.Init(rate, 40.f / rate);
		sample_rate = rate;
	}
};

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Tides* amp = (Tides*) new Tides(rate);


	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Tides* amp = (Tides*)instance;

	switch ((PortIndex)port) {

		case RANGE_PARAM:
			amp->range_param = (const float*)data;
			break;
		case MODE_PARAM:
			amp->mode_param = (const float*)data;
			break;
		case RAMP_PARAM:
			amp->ramp_param = (const float*)data;
			break;
		case FREQUENCY_PARAM:
			amp->frequency_param = (const float*)data;
			break;
		case SHAPE_PARAM:
			amp->shape_param = (const float*)data;
			break;
		case SMOOTHNESS_PARAM:
			amp->smoothness_param = (const float*)data;
			break;
		case SLOPE_PARAM:
			amp->slope_param = (const float*)data;
			break;
		case SHIFT_PARAM:
			amp->shift_param = (const float*)data;
			break;
		case SLOPE_CV_PARAM:
			amp->slope_cv_param = (const float*)data;
			break;
		case FREQUENCY_CV_PARAM:
			amp->frequency_cv_param = (const float*)data;
			break;
		case SMOOTHNESS_CV_PARAM:
			amp->smoothness_cv_param = (const float*)data;
			break;
		case SHAPE_CV_PARAM:
			amp->shape_cv_param = (const float*)data;
			break;
		case SHIFT_CV_PARAM:
			amp->shift_cv_param = (const float*)data;
			break;
		case SLOPE_INPUT:
			amp->slope_input = (const float*)data;
			break;
		case FREQUENCY_INPUT:
			amp->frequency_input = (const float*)data;
			break;
		case V_OCT_INPUT:
			amp->v_oct_input = (const float*)data;
			break;
		case SMOOTHNESS_INPUT:
			amp->smoothness_input = (const float*)data;
			break;
		case SHAPE_INPUT:
			amp->shape_input = (const float*)data;
			break;
		case SHIFT_INPUT:
			amp->shift_input = (const float*)data;
			break;
		case TRIG_INPUT:
			amp->trig_input = (const float*)data;
			break;
		case CLOCK_INPUT:
			amp->clock_input = (const float*)data;
			break;
// outs
		case OUT1:
			amp->out1 = (float*)data;
			break;
		case OUT2:
			amp->out2 = (float*)data;
			break;
		case OUT3:
			amp->out3 = (float*)data;
			break;
		case OUT4:
			amp->out4 = (float*)data;
			break;
		case TEMPO_OUT:
			amp->tempo_out = (float*)data;
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
	Tides* const amp = (Tides*)instance;

	const float range_param = *(amp->range_param);
	const float mode_param = *(amp->mode_param);
	const float ramp_param = *(amp->ramp_param);
	const float frequency_param = *(amp->frequency_param);
	const float shape_param = *(amp->shape_param);
	const float smoothness_param = *(amp->smoothness_param);
	const float slope_param = *(amp->slope_param);
	const float shift_param = *(amp->shift_param);
	const float slope_cv_param = *(amp->slope_cv_param);
	const float frequency_cv_param = *(amp->frequency_cv_param);
	const float smoothness_cv_param = *(amp->smoothness_cv_param);
	const float shape_cv_param = *(amp->shape_cv_param);
	const float shift_cv_param = *(amp->shift_cv_param);
	// CV inputs
	const float* slope_input = amp->slope_input;
	const float* frequency_input = amp->frequency_input;
	const float* v_oct_input = amp->v_oct_input;
	const float* smoothness_input = amp->smoothness_input;
	const float* shape_input = amp->shape_input;
	const float* shift_input = amp->shift_input;
	const float* trig_input = amp->trig_input;
	const float* clock_input = amp->clock_input;
	// outs
	float* out1 = amp->out1;
	float* out2 = amp->out2;
	float* out3 = amp->out3;
	float* out4 = amp->out4;
	float* tempo_out = amp->tempo_out;

	const bool trig_is_connected = trig_input[0] > -40.0f;
	const bool clock_is_connected = clock_input[0] > -40.0f;


	// State
	int range = (int) range_param % 3;
	tides::OutputMode output_mode = (tides::OutputMode)((int) (mode_param) % 4);
	tides::RampMode ramp_mode = (tides::RampMode)((int) (ramp_param) % 3); 
	tides::Range range_mode = (range_param < 2.0f) ? tides::RANGE_CONTROL : tides::RANGE_AUDIO;

	uint32_t block_size = tides::kBlockSize; // 8
	if (n_samples < block_size){
		block_size = n_samples;	
	}

	tides::PolySlopeGenerator::OutputSample out[tides::kBlockSize] = {};

	uint32_t pos = 0;
	float frequency;
	while (pos < n_samples){
		uint32_t s = pos;
		// Input gates
		for (uint32_t i = 0; i < block_size; i++) {
			amp->trig_flags[i] = stmlib::ExtractGateFlags(amp->previous_trig_flag, trig_input[s] > 0.33f);
			amp->previous_trig_flag = amp->trig_flags[i];
			amp->clock_flags[i] = stmlib::ExtractGateFlags(amp->previous_clock_flag, clock_input[s] > 0.33);
			amp->previous_clock_flag = amp->clock_flags[i];
		}

		float note = clamp(frequency_param + v_oct_input[s] * 60.f, -96.f, 96.f);
		float fm = clamp(frequency_cv_param * frequency_input[s] * 60.f, -96.f, 96.f);
		float transposition = note + fm;

		float ramp[block_size];

		if (clock_is_connected) {
			if (amp->must_reset_ramp_extractor) {
				amp->ramp_extractor.Reset();
			}

			tides::Ratio r = amp->ratio_index_quantizer.Lookup(kRatios, 0.5f + transposition * 0.0105f, 20);
			frequency = amp->ramp_extractor.Process(
					range_mode == tides::RANGE_AUDIO,
					range_mode == tides::RANGE_AUDIO && ramp_mode == tides::RAMP_MODE_AR,
					r,
					amp->clock_flags,
					ramp,
					block_size);
			amp->must_reset_ramp_extractor = false;
		}
		else {
			frequency = kRootScaled[range] / amp->sample_rate * stmlib::SemitonesToRatio(transposition);
			amp->must_reset_ramp_extractor = true;
		}

		// Get parameters
		float slope = clamp(slope_param + slope_cv_param * slope_input[s], 0.f, 1.f);
		float shape = clamp(shape_param + shape_cv_param * shape_input[s], 0.f, 1.f);
		float smoothness = clamp(smoothness_param + smoothness_cv_param * smoothness_input[s], 0.f, 1.f);
		float shift = clamp(shift_param + shift_cv_param * shift_input[s], 0.f, 1.f);

		if (output_mode != amp->previous_output_mode) {
			amp->poly_slope_generator.Reset();
			amp->previous_output_mode = output_mode;
		}

		// Render generator
		amp->poly_slope_generator.Render(
				ramp_mode,
				output_mode,
				range_mode,
				frequency,
				slope,
				shape,
				smoothness,
				shift,
				amp->trig_flags,
				!trig_is_connected && clock_is_connected ? ramp : NULL,
				out,
				block_size);

		// Outputs
		//
		for (uint32_t i = 0; i < block_size; i++) {
			out1[pos+i] =  out[i].channel[0];
			out2[pos+i] =  out[i].channel[1];
			out3[pos+i] =  out[i].channel[2];
			out4[pos+i] =  out[i].channel[3];
		}

		pos = pos + block_size;
		if (pos+block_size > n_samples){
			block_size = n_samples - pos;
		}
	}
	tempo_out[0] = frequency * amp->sample_rate * 15; // 60 / 4 ;
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
	Tides* const amp = (Tides*)instance;
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
	TIDES_URI,
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

