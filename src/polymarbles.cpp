#include "lv2/core/lv2.h"

/** Include standard C headers */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "marbles/random/random_generator.h"
#include "marbles/random/random_stream.h"
#include "marbles/random/t_generator.h"
#include "marbles/random/x_y_generator.h"
#include "marbles/note_filter.h"

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))
#define LENGTHOF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define MARBLES_URI "http://polyeffects.com/lv2/polymarbles"


static const int MAX_BLOCK_SIZE = 8;


static const marbles::Scale preset_scales[6] = {
	// C major
	{
		1.0f,
		12,
		{
			{ 0.0000f, 255 },  // C
			{ 0.0833f, 16 },   // C#
			{ 0.1667f, 96 },   // D
			{ 0.2500f, 24 },   // D#
			{ 0.3333f, 128 },  // E
			{ 0.4167f, 64 },   // F
			{ 0.5000f, 8 },    // F#
			{ 0.5833f, 192 },  // G
			{ 0.6667f, 16 },   // G#
			{ 0.7500f, 96 },   // A
			{ 0.8333f, 24 },   // A#
			{ 0.9167f, 128 },  // B
		}
	},

	// C minor
	{
		1.0f,
		12,
		{
			{ 0.0000f, 255 },  // C
			{ 0.0833f, 16 },   // C#
			{ 0.1667f, 96 },   // D
			{ 0.2500f, 128 },  // Eb
			{ 0.3333f, 8 },    // E
			{ 0.4167f, 64 },   // F
			{ 0.5000f, 4 },    // F#
			{ 0.5833f, 192 },  // G
			{ 0.6667f, 16 },   // G#
			{ 0.7500f, 96 },   // A
			{ 0.8333f, 128 },  // Bb
			{ 0.9167f, 16 },   // B
		}
	},

	// Pentatonic
	{
		1.0f,
		12,
		{
			{ 0.0000f, 255 },  // C
			{ 0.0833f, 4 },    // C#
			{ 0.1667f, 96 },   // D
			{ 0.2500f, 4 },    // Eb
			{ 0.3333f, 4 },    // E
			{ 0.4167f, 140 },  // F
			{ 0.5000f, 4 },    // F#
			{ 0.5833f, 192 },  // G
			{ 0.6667f, 4 },    // G#
			{ 0.7500f, 96 },   // A
			{ 0.8333f, 4 },    // Bb
			{ 0.9167f, 4 },    // B
		}
	},

	// Pelog
	{
		1.0f,
		7,
		{
			{ 0.0000f, 255 },  // C
			{ 0.1275f, 128 },  // Db+
			{ 0.2625f, 32 },  // Eb-
			{ 0.4600f, 8 },    // F#-
			{ 0.5883f, 192 },  // G
			{ 0.7067f, 64 },  // Ab
			{ 0.8817f, 16 },    // Bb+
		}
	},

	// Raag Bhairav That
	{
		1.0f,
		12,
		{
			{ 0.0000f, 255 }, // ** Sa
			{ 0.0752f, 128 }, // ** Komal Re
			{ 0.1699f, 4 },   //    Re
			{ 0.2630f, 4 },   //    Komal Ga
			{ 0.3219f, 128 }, // ** Ga
			{ 0.4150f, 64 },  // ** Ma
			{ 0.4918f, 4 },   //    Tivre Ma
			{ 0.5850f, 192 }, // ** Pa
			{ 0.6601f, 64 },  // ** Komal Dha
			{ 0.7549f, 4 },   //    Dha
			{ 0.8479f, 4 },   //    Komal Ni
			{ 0.9069f, 64 },  // ** Ni
		}
	},

	// Raag Shri
	{
		1.0f,
		12,
		{
			{ 0.0000f, 255 }, // ** Sa
			{ 0.0752f, 4 },   //    Komal Re
			{ 0.1699f, 128 }, // ** Re
			{ 0.2630f, 64 },  // ** Komal Ga
			{ 0.3219f, 4 },   //    Ga
			{ 0.4150f, 128 }, // ** Ma
			{ 0.4918f, 4 },   //    Tivre Ma
			{ 0.5850f, 192 }, // ** Pa
			{ 0.6601f, 4 },   //    Komal Dha
			{ 0.7549f, 64 },  // ** Dha
			{ 0.8479f, 128 }, // ** Komal Ni
			{ 0.9069f, 4 },   //    Ni
		}
	},
};

typedef enum {
	// cv inputs
	T_BIAS_INPUT,
	X_BIAS_INPUT,
	T_CLOCK_INPUT,
	T_RATE_INPUT,
	T_JITTER_INPUT,
	DEJA_VU_INPUT,
	X_STEPS_INPUT,
	X_SPREAD_INPUT,
	X_CLOCK_INPUT,
	// control rate params
	T_DEJA_VU_PARAM,
	X_DEJA_VU_PARAM,
	DEJA_VU_PARAM,
	T_RATE_PARAM,
	X_SPREAD_PARAM,
	T_MODE_PARAM,
	X_MODE_PARAM,
	DEJA_VU_LENGTH_PARAM,
	T_BIAS_PARAM,
	X_BIAS_PARAM,
	T_RANGE_PARAM,
	X_RANGE_PARAM,
	EXTERNAL_PARAM,
	T_JITTER_PARAM,
	X_STEPS_PARAM,
	// outputs
	T1_OUTPUT,
	T2_OUTPUT,
	T3_OUTPUT,
	Y_OUTPUT,
	X1_OUTPUT,
	X2_OUTPUT,
	X3_OUTPUT,
	X_SCALE_PARAM,
	Y_DIVIDER_PARAM,
	X_CLOCK_SOURCE_INTERNAL_PARAM,
	// added
	Y_SPREAD_PARAM,
	Y_BIAS_PARAM,
	Y_STEPS_PARAM,
} PortIndex;

struct Marbles {

	marbles::RandomGenerator random_generator;
	marbles::RandomStream random_stream;
	marbles::TGenerator t_generator;
	marbles::XYGenerator xy_generator;
	marbles::NoteFilter note_filter;

	// State
	float srate;

	// Buffers
	stmlib::GateFlags t_clocks[MAX_BLOCK_SIZE] = {};
	stmlib::GateFlags last_t_clock = 0;
	stmlib::GateFlags xy_clocks[MAX_BLOCK_SIZE] = {};
	stmlib::GateFlags last_xy_clock = 0;
	float ramp_master[MAX_BLOCK_SIZE] = {};
	float ramp_external[MAX_BLOCK_SIZE] = {};
	float ramp_slave[2][MAX_BLOCK_SIZE] = {};
	bool gates[MAX_BLOCK_SIZE * 2] = {};

	// port buffers
	// cv inputs
	const float* t_bias_input;
	const float* x_bias_input;
	const float* t_clock_input;
	const float* t_rate_input;
	const float* t_jitter_input;
	const float* deja_vu_input;
	const float* x_steps_input;
	const float* x_spread_input;
	const float* x_clock_input;
	// control rate params
	const float* t_deja_vu_param;
	const float* x_deja_vu_param;
	const float* deja_vu_param;
	const float* t_rate_param;
	const float* x_spread_param;
	const float* t_mode_param;
	const float* x_mode_param;
	const float* deja_vu_length_param;
	const float* t_bias_param;
	const float* x_bias_param;
	const float* t_range_param;
	const float* x_range_param;
	const float* external_param;
	const float* t_jitter_param;
	const float* x_steps_param;

	const float* x_scale_param;
	const float* y_divider_param;
	const float* x_clock_source_internal_param;

	const float* y_spread_param;
	const float* y_bias_param;
	const float* y_steps_param;


	// outputs
	float* t1_output;
	float* t2_output;
	float* t3_output;
	float* y_output;
	float* x1_output;
	float* x2_output;
	float* x3_output;

	Marbles(int samplerate) {
		/* config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS); */
		/* configParam(T_DEJA_VU_PARAM, 0.0, 1.0, 0.0, "t deja vu"); */
		/* configParam(X_DEJA_VU_PARAM, 0.0, 1.0, 0.0, "X deja vu"); */
		/* configParam(DEJA_VU_PARAM, 0.0, 1.0, 0.5, "Deja vu probability"); */
		/* configParam(T_RATE_PARAM, -1.0, 1.0, 0.0, "Clock rate"); */
		/* configParam(X_SPREAD_PARAM, 0.0, 1.0, 0.5, "Probability distribution"); */
		/* configParam(T_MODE_PARAM, 0.0, 1.0, 0.0, "t mode"); */
		/* configParam(X_MODE_PARAM, 0.0, 1.0, 0.0, "X mode"); */
		/* configParam(DEJA_VU_LENGTH_PARAM, 0.0, 1.0, 0.0, "Loop length"); */
		/* configParam(T_BIAS_PARAM, 0.0, 1.0, 0.5, "Gate bias"); */
		/* configParam(X_BIAS_PARAM, 0.0, 1.0, 0.5, "Distribution bias"); */
		/* configParam(T_RANGE_PARAM, 0.0, 1.0, 0.0, "Clock range mode"); */
		/* configParam(X_RANGE_PARAM, 0.0, 1.0, 0.0, "Output voltage range mode"); */
		/* configParam(EXTERNAL_PARAM, 0.0, 1.0, 0.0, "External processing mode"); */
		/* configParam(T_JITTER_PARAM, 0.0, 1.0, 0.0, "Randomness amount"); */
		/* configParam(X_STEPS_PARAM, 0.0, 1.0, 0.5, "Smoothness"); */

		random_generator.Init(1);
		random_stream.Init(&random_generator);
		note_filter.Init();
		srate = samplerate;
		onSampleRateChange();
	}


	void onSampleRateChange() {
		t_generator.Init(&random_stream, srate);
		xy_generator.Init(&random_stream, srate);

		// Set scales
		for (int i = 0; i < 6; i++) {
			xy_generator.LoadScale(i, preset_scales[i]);
		}
	}
};

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Marbles* amp = (Marbles*) new Marbles(rate);


	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Marbles* amp = (Marbles*)instance;

	switch ((PortIndex)port) {
		case T_BIAS_INPUT:
			amp->t_bias_input = (const float*)data;
			break;
		case X_BIAS_INPUT:
			amp->x_bias_input = (const float*)data;
			break;
		case T_CLOCK_INPUT:
			amp->t_clock_input = (const float*)data;
			break;
		case T_RATE_INPUT:
			amp->t_rate_input = (const float*)data;
			break;
		case T_JITTER_INPUT:
			amp->t_jitter_input = (const float*)data;
			break;
		case DEJA_VU_INPUT:
			amp->deja_vu_input = (const float*)data;
			break;
		case X_STEPS_INPUT:
			amp->x_steps_input = (const float*)data;
			break;
		case X_SPREAD_INPUT:
			amp->x_spread_input = (const float*)data;
			break;
		case X_CLOCK_INPUT:
			amp->x_clock_input = (const float*)data;
			break;
	// control rate params
		case T_DEJA_VU_PARAM:
			amp->t_deja_vu_param = (const float*)data;
			break;
		case X_DEJA_VU_PARAM:
			amp->x_deja_vu_param = (const float*)data;
			break;
		case DEJA_VU_PARAM:
			amp->deja_vu_param = (const float*)data;
			break;
		case T_RATE_PARAM:
			amp->t_rate_param = (const float*)data;
			break;
		case X_SPREAD_PARAM:
			amp->x_spread_param = (const float*)data;
			break;
		case T_MODE_PARAM:
			amp->t_mode_param = (const float*)data;
			break;
		case X_MODE_PARAM:
			amp->x_mode_param = (const float*)data;
			break;
		case DEJA_VU_LENGTH_PARAM:
			amp->deja_vu_length_param = (const float*)data;
			break;
		case T_BIAS_PARAM:
			amp->t_bias_param = (const float*)data;
			break;
		case X_BIAS_PARAM:
			amp->x_bias_param = (const float*)data;
			break;
		case T_RANGE_PARAM:
			amp->t_range_param = (const float*)data;
			break;
		case X_RANGE_PARAM:
			amp->x_range_param = (const float*)data;
			break;
		case EXTERNAL_PARAM:
			amp->external_param = (const float*)data;
			break;
		case T_JITTER_PARAM:
			amp->t_jitter_param = (const float*)data;
			break;
		case X_STEPS_PARAM:
			amp->x_steps_param = (const float*)data;
			break;
	// outputs
		case T1_OUTPUT:
			amp->t1_output = (float*)data;
			break;
		case T2_OUTPUT:
			amp->t2_output = (float*)data;
			break;
		case T3_OUTPUT:
			amp->t3_output = (float*)data;
			break;
		case Y_OUTPUT:
			amp->y_output = (float*)data;
			break;
		case X1_OUTPUT:
			amp->x1_output = (float*)data;
			break;
		case X2_OUTPUT:
			amp->x2_output = (float*)data;
			break;
		case X3_OUTPUT:
			amp->x3_output = (float*)data;
			break;

		case X_SCALE_PARAM:
			amp->x_scale_param = (const float*)data;
			break;
		case Y_DIVIDER_PARAM:
			amp->y_divider_param = (const float*)data;
			break;
		case X_CLOCK_SOURCE_INTERNAL_PARAM:
			amp->x_clock_source_internal_param = (const float*)data;
			break;

		case Y_SPREAD_PARAM:
			amp->y_spread_param = (const float*)data;
			break;
		case Y_BIAS_PARAM:
			amp->y_bias_param = (const float*)data;
			break;
		case Y_STEPS_PARAM:
			amp->y_steps_param = (const float*)data;
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
	Marbles* const amp = (Marbles*)instance;

	const float* t_bias_input = amp->t_bias_input;
	const float* x_bias_input = amp->x_bias_input;
	const float* t_clock_input = amp->t_clock_input;
	const float* t_rate_input = amp->t_rate_input;
	const float* t_jitter_input = amp->t_jitter_input;
	const float* deja_vu_input = amp->deja_vu_input;
	const float* x_steps_input = amp->x_steps_input;
	const float* x_spread_input = amp->x_spread_input;
	const float* x_clock_input = amp->x_clock_input;
	// control rate params
	const float t_deja_vu_param = *(amp->t_deja_vu_param);
	const float x_deja_vu_param = *(amp->x_deja_vu_param);
	const float deja_vu_param = *(amp->deja_vu_param);
	const float t_rate_param = *(amp->t_rate_param);
	const float x_spread_param = *(amp->x_spread_param);
	const float t_mode_param = *(amp->t_mode_param);
	const float x_mode_param = *(amp->x_mode_param);
	const float deja_vu_length_param = *(amp->deja_vu_length_param);
	const float t_bias_param = *(amp->t_bias_param);
	const float x_bias_param = *(amp->x_bias_param);
	const float t_range_param = *(amp->t_range_param);
	const float x_range_param = *(amp->x_range_param);
	const float external_param = *(amp->external_param);
	const float t_jitter_param = *(amp->t_jitter_param);
	const float x_steps_param = *(amp->x_steps_param);

	const float x_scale_param = *(amp->x_scale_param);
	const float y_divider_param= *(amp->y_divider_param);
	const float x_clock_source_internal_param = *(amp->x_clock_source_internal_param );

	const float y_spread_param = *(amp->y_spread_param);
	const float y_bias_param = *(amp->y_bias_param);
	const float y_steps_param = *(amp->y_steps_param);

	// outputs
	float* t1_output = amp->t1_output;
	float* t2_output = amp->t2_output;
	float* t3_output = amp->t3_output;
	float* y_output = amp->y_output;
	float* x1_output = amp->x1_output;
	float* x2_output = amp->x2_output;
	float* x3_output = amp->x3_output;

	float voltages[MAX_BLOCK_SIZE * 4] = {};
	bool t_deja_vu;
	bool x_deja_vu;
	int t_mode;
	int x_mode;
	int t_range;
	int x_range;
	bool external;

	int x_scale = ((int) x_scale_param) % 5; 
	int y_divider_index = ((int) y_divider_param) % 12; 
	int x_clock_source_internal = ((int) x_clock_source_internal_param) % 3;


	// Buttons
	t_deja_vu = (bool) t_deja_vu_param;
	x_deja_vu = (bool) x_deja_vu_param;
	t_mode = (int) (t_mode_param) % 3;
	x_mode = (int) (x_mode_param) % 3;
	t_range = (int) (t_range_param) % 3;
	x_range = (int) (x_range_param) % 3;
	external = external_param;

	// convert t_rate_param BPM input to number between -1 and 1 limit BPM to 4-3840
	float t_rate_param_converted = logf(clamp(t_rate_param, 4.f, 3840.f)/60.0f)/(5*logf(2.0f))-0.2f;

	uint32_t block_size = 5;
	if (n_samples < block_size){
		block_size = n_samples;	
	}

	// Process block
	uint32_t pos = 0;
	while (pos < n_samples){
	/* for (uint32_t s = 0; s < n_samples; s++) { */ 
		uint32_t s = pos;

		for (int blockIndex = 0; blockIndex < block_size; blockIndex++){
			// Clocks
			bool t_gate = t_clock_input[pos+blockIndex] >= 1.0f;  
			amp->last_t_clock = stmlib::ExtractGateFlags(amp->last_t_clock, t_gate);
			amp->t_clocks[blockIndex] = amp->last_t_clock;

			bool x_gate = x_clock_input[pos+blockIndex] >= 1.0f; 
			amp->last_xy_clock = stmlib::ExtractGateFlags(amp->last_xy_clock, x_gate);
			amp->xy_clocks[blockIndex] = amp->last_xy_clock;
		}

		// Ramps

		marbles::Ramps ramps;
		ramps.master = amp->ramp_master;
		ramps.external = amp->ramp_external;
		ramps.slave[0] = amp->ramp_slave[0];
		ramps.slave[1] = amp->ramp_slave[1];

		float deja_vu = clamp(deja_vu_param + deja_vu_input[s], 0.0f, 1.0f);
		static const int loop_length[] = {
			1, 1, 1, 2, 2,
			2, 2, 2, 3, 3,
			3, 3, 4, 4, 4,
			4, 4, 5, 5, 6,
			6, 6, 7, 7, 8,
			8, 8, 10, 10, 12,
			12, 12, 14, 14, 16,
			16
		};
		float deja_vu_length_index = deja_vu_length_param * (LENGTHOF(loop_length) - 1);
		int deja_vu_length = loop_length[(int) roundf(deja_vu_length_index)];

		// Set up TGenerator

		bool t_external_clock = t_clock_input[s] > -40.0f;

		amp->t_generator.set_model((marbles::TGeneratorModel) t_mode);
		amp->t_generator.set_range((marbles::TGeneratorRange) t_range);
		float t_rate = 60.f * (t_rate_param_converted + t_rate_input[s]);
		amp->t_generator.set_rate(t_rate);
		float t_bias = clamp(t_bias_param + t_bias_input[s], 0.f, 1.f);
		amp->t_generator.set_bias(t_bias);
		float t_jitter = clamp(t_jitter_param + t_jitter_input[s], 0.f, 1.f);
		amp->t_generator.set_jitter(t_jitter);
		amp->t_generator.set_deja_vu(t_deja_vu ? deja_vu : 0.f);
		amp->t_generator.set_length(deja_vu_length);
		// TODO
		amp->t_generator.set_pulse_width_mean(0.f);
		amp->t_generator.set_pulse_width_std(0.f);

		amp->t_generator.Process(t_external_clock, amp->t_clocks, ramps, amp->gates, block_size);

		// Set up XYGenerator

		marbles::ClockSource x_clock_source = (marbles::ClockSource) x_clock_source_internal;
		if (x_clock_input[s] > -40.0f){
			x_clock_source = marbles::CLOCK_SOURCE_EXTERNAL;
		}

		marbles::GroupSettings x;
		x.control_mode = (marbles::ControlMode) x_mode;
		x.voltage_range = (marbles::VoltageRange) x_range;
		// TODO Fix the scaling
		float note_cv = 0.5f * (x_spread_param + x_spread_input[s]);
		float u = amp->note_filter.Process(0.5f * (note_cv + 1.f));
		x.register_mode = external;
		x.register_value = u;

		float x_spread = clamp(x_spread_param + x_spread_input[s], 0.f, 1.f);
		x.spread = x_spread;
		float x_bias = clamp(x_bias_param + x_bias_input[s], 0.f, 1.f);
		x.bias = x_bias;
		float x_steps = clamp(x_steps_param + x_steps_input[s], 0.f, 1.f);
		x.steps = x_steps;
		x.deja_vu = x_deja_vu ? deja_vu : 0.f;
		x.length = deja_vu_length;
		x.ratio.p = 1;
		x.ratio.q = 1;
		x.scale_index = x_scale;

		marbles::GroupSettings y;
		y.control_mode = marbles::CONTROL_MODE_IDENTICAL;
		// TODO
		y.voltage_range = (marbles::VoltageRange) x_range;
		y.register_mode = false;
		y.register_value = 0.0f;
		float y_spread = clamp(y_spread_param, 0.f, 1.f);
		float y_bias = clamp(y_bias_param, 0.f, 1.f);
		float y_steps = clamp(y_steps_param, 0.f, 1.f);
		y.spread = y_spread;
		y.bias = y_bias;
		y.steps = y_steps;
		y.deja_vu = 0.0f;
		y.length = 1;
		static const marbles::Ratio y_divider_ratios[] = {
			{ 1, 64 },
			{ 1, 48 },
			{ 1, 32 },
			{ 1, 24 },
			{ 1, 16 },
			{ 1, 12 },
			{ 1, 8 },
			{ 1, 6 },
			{ 1, 4 },
			{ 1, 3 },
			{ 1, 2 },
			{ 1, 1 },
		};
		y.ratio = y_divider_ratios[y_divider_index];
		y.scale_index = x_scale;

		amp->xy_generator.Process(x_clock_source, x, y, amp->xy_clocks, ramps, voltages, block_size);
		

		// 
		for (int blockIndex = 0; blockIndex < block_size; blockIndex++){
			int i = pos+blockIndex;
			t1_output[i] = amp->gates[blockIndex * 2 + 0] ? 1.f : 0.f;
			t2_output[i] = (amp->ramp_master[blockIndex] < 0.5f) ? 1.f : 0.f;
			t3_output[i] = amp->gates[blockIndex * 2 + 1] ? 1.f : 0.f;
			
			if (x_range == 0){
				x1_output[i] = voltages[blockIndex * 4 + 0];  
				x2_output[i] = voltages[blockIndex * 4 + 1];
				x3_output[i] = voltages[blockIndex * 4 + 2];
				y_output[i] = voltages[blockIndex * 4 + 3];
			}
			else
			{
				x1_output[i] = voltages[blockIndex * 4 + 0] / 5.0f; 
				x2_output[i] = voltages[blockIndex * 4 + 1] / 5.0f;
				x3_output[i] = voltages[blockIndex * 4 + 2] / 5.0f;
				y_output[i] = voltages[blockIndex * 4 + 3] / 5.0f;
			
			}
		}

		pos = pos + block_size;
		if (pos+block_size > n_samples){
			block_size = n_samples - pos;
		}
	}

};




/* struct MarblesWidget : ModuleWidget { */
/* 	void appendContextMenu(Menu* menu) override { */
/* 		Marbles* module = dynamic_cast<Marbles*>(this->module); */

/* 		struct ScaleItem : MenuItem { */
/* 			Marbles* module; */
/* 			int scale; */
/* 			void onAction(const event::Action& e) override { */
/* 				module->x_scale = scale; */
/* 			} */
/* 		}; */

/* 		menu->addChild(new MenuSeparator); */
/* 		menu->addChild(createMenuLabel("Scales")); */
/* 		const std::string scaleLabels[] = { */
/* 			"Major", */
/* 			"Minor", */
/* 			"Pentatonic", */
/* 			"Pelog", */
/* 			"Raag Bhairav That", */
/* 			"Raag Shri", */
/* 		}; */
/* 		for (int i = 0; i < (int) LENGTHOF(scaleLabels); i++) { */
/* 			ScaleItem* item = createMenuItem<ScaleItem>(scaleLabels[i], CHECKMARK(module->x_scale == i)); */
/* 			item->module = module; */
/* 			item->scale = i; */
/* 			menu->addChild(item); */
/* 		} */

/* 		struct XClockSourceInternal : MenuItem { */
/* 			Marbles* module; */
/* 			int source; */
/* 			void onAction(const event::Action& e) override { */
/* 				module->x_clock_source_internal = source; */
/* 			} */
/* 		}; */

/* 		menu->addChild(new MenuSeparator); */
/* 		menu->addChild(createMenuLabel("Internal X clock source")); */
/* 		const std::string sourceLabels[] = { */
/* 			"T₁ → X₁, T₂ → X₂, T₃ → X₃", */
/* 			"T₁ → X₁, X₂, X₃", */
/* 			"T₂ → X₁, X₂, X₃", */
/* 			"T₃ → X₁, X₂, X₃", */
/* 		}; */
/* 		for (int i = 0; i < (int) LENGTHOF(sourceLabels); i++) { */
/* 			XClockSourceInternal* item = createMenuItem<XClockSourceInternal>(sourceLabels[i], CHECKMARK(module->x_clock_source_internal == i)); */
/* 			item->module = module; */
/* 			item->source = i; */
/* 			menu->addChild(item); */
/* 		} */

/* 		struct YDividerIndexItem : MenuItem { */
/* 			Marbles* module; */
/* 			int index; */
/* 			void onAction(const event::Action& e) override { */
/* 				module->y_divider_index = index; */
/* 			} */
/* 		}; */

/* 		struct YDividerItem : MenuItem { */
/* 			Marbles* module; */
/* 			Menu* createChildMenu() override { */
/* 				Menu* menu = new Menu(); */
/* 				const std::string yDividerRatioLabels[] = { */
/* 					"1/64", */
/* 					"1/48", */
/* 					"1/32", */
/* 					"1/24", */
/* 					"1/16", */
/* 					"1/12", */
/* 					"1/8", */
/* 					"1/6", */
/* 					"1/4", */
/* 					"1/3", */
/* 					"1/2", */
/* 					"1", */
/* 				}; */
/* 				for (int i = 0; i < (int) LENGTHOF(yDividerRatioLabels); i++) { */
/* 					YDividerIndexItem* item = createMenuItem<YDividerIndexItem>(yDividerRatioLabels[i], CHECKMARK(module->y_divider_index == i)); */
/* 					item->module = module; */
/* 					item->index = i; */
/* 					menu->addChild(item); */
/* 				} */
/* 				return menu; */
/* 			} */
/* 		}; */

/* 		menu->addChild(new MenuSeparator); */
/* 		YDividerItem* yDividerItem = createMenuItem<YDividerItem>("Y divider ratio"); */
/* 		yDividerItem->module = module; */
/* 		menu->addChild(yDividerItem); */
/* 	} */
/* }; */

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
	Marbles* const amp = (Marbles*)instance;
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
	MARBLES_URI,
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
