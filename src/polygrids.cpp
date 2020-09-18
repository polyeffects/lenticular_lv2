// Copyright 2020 Loki Davison <loki@polyeffects.com>
// A port of Topograph to LV2, which is a port of "Mutable Instruments Grids" for VCV Rack
//
// Author: Dale Johnson
// Contact: valley.audio.soft@gmail.com
// Date: 5/12/2017
//
// Original author: Olivier Gillet (ol.gillet@gmail.com)
// https://github.com/pichenettes/eurorack/tree/master/grids
// Copyright 2012 Olivier Gillet.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Metronome.hpp"
#include "Oneshot.hpp"
#include "TopographPatternGenerator.hpp"
#include "lv2/core/lv2.h"

/** Include standard C headers */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define clamp(x, lower, upper) (fmax(lower, fmin(x, upper)))

#define GRIDS_URI "http://polyeffects.com/lv2/polygrids"
#define SAMPLE_RATE 48000

/**
   In code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability.
*/
typedef enum {
        RESET_BUTTON_PARAM = 0,
        RUN_BUTTON_PARAM,
        TEMPO_PARAM,
        MAPX_PARAM,
        MAPY_PARAM,
        CHAOS_PARAM,
        BD_DENS_PARAM,
        SN_DENS_PARAM,
        HH_DENS_PARAM,
        SWING_PARAM,
        CLOCK_INPUT,
        RESET_INPUT,
        MAPX_CV,
        MAPY_CV,
        CHAOS_CV,
        BD_FILL_CV,
        SN_FILL_CV,
        HH_FILL_CV,
        SWING_CV,
        RUN_INPUT,
        BD_OUTPUT,
        SN_OUTPUT,
        HH_OUTPUT,
        BD_ACC_OUTPUT,
        SN_ACC_OUTPUT,
        HH_ACC_OUTPUT,
} PortIndex;

enum RunMode {
	TOGGLE,
	MOMENTARY
};

enum TriggerOutputMode {
	PULSE,
	GATE
};
/**
   Every plugin defines a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method.  In this simple plugin, only port buffers need to be
   stored, since there is no additional instance data.
*/
typedef struct {

	const float* reset_button_param;
	const float* run_button_param;
	const float* tempo_param;
	const float* mapx_param;
	const float* mapy_param;
	const float* chaos_param;
	const float* bd_dens_param;
	const float* sn_dens_param;
	const float* hh_dens_param;
	const float* swing_param;
	const float* clock_input;
	const float* reset_input;
	const float* mapx_cv;
	const float* mapy_cv;
	const float* chaos_cv;
	const float* bd_fill_cv;
	const float* sn_fill_cv;
	const float* hh_fill_cv;
	const float* swing_cv;
	const float* run_input;
	// out
	float* bd_output;
	float* sn_output;
	float* hh_output;
	float* bd_acc_output;
	float* sn_acc_output;
	float* hh_acc_output;

    Metronome* metro;
    PatternGenerator* grids;
    uint8_t numTicks;
    bool initExtReset = true;
    int running = 0;
    bool extClock = false;
    long elapsedTicks = 0;

    float tempo = 120.0f;

    uint8_t state = 0;

    // Drum Triggers
    Oneshot drumTriggers[6];
    bool gateState[6];

    enum SequencerMode {
        HENRI,
        OLIVIER,
        EUCLIDEAN
    };
    SequencerMode sequencerMode = HENRI;
    int inEuclideanMode = 0;

    TriggerOutputMode triggerOutputMode = GATE;

    enum AccOutputMode {
        INDIVIDUAL_ACCENTS,
        ACC_CLK_RST
    };
    AccOutputMode accOutputMode = INDIVIDUAL_ACCENTS;

    enum ExtClockResolution {
        EXTCLOCK_RES_4_PPQN,
        EXTCLOCK_RES_8_PPQN,
        EXTCLOCK_RES_24_PPQN,
    };
    ExtClockResolution extClockResolution = EXTCLOCK_RES_24_PPQN;

    enum ChaosKnobMode {
        CHAOS,
        SWING
    };
    ChaosKnobMode chaosKnobMode = CHAOS;

    RunMode runMode = MOMENTARY; //TOGGLE; 

    int panelStyle;
    int textVisible = 1;

} Grids;


static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	/* Grids* amp = (Grids*)malloc(sizeof(Grids)); */
	Grids* amp = new Grids();
	/* for (int i = 0; i < 16; i++) { */
	/* const int memLen = 16384; */
	/* amp->shared_buf = new uint8_t[memLen](); */

	amp->grids = new PatternGenerator();
	amp->metro = new Metronome(120, SAMPLE_RATE, 24.0, 0.0);
	amp->numTicks = ticks_granularity[2];
	srand(time(NULL));

	for(int i = 0; i < 6; ++i) {
		amp->drumTriggers[i] = Oneshot(0.001, SAMPLE_RATE);
		amp->gateState[i] = false;
	}


	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Grids* amp = (Grids*)instance;

    switch ((PortIndex)port) {
		case RESET_BUTTON_PARAM:
			amp->reset_button_param = (const float*)data;
			break;
		case RUN_BUTTON_PARAM:
			amp->run_button_param = (const float*)data;
			break;
		case TEMPO_PARAM:
			amp->tempo_param = (const float*)data;
			break;
		case MAPX_PARAM:
			amp->mapx_param = (const float*)data;
			break;
		case MAPY_PARAM:
			amp->mapy_param = (const float*)data;
			break;
		case CHAOS_PARAM:
			amp->chaos_param = (const float*)data;
			break;
		case BD_DENS_PARAM:
			amp->bd_dens_param = (const float*)data;
			break;
		case SN_DENS_PARAM:
			amp->sn_dens_param = (const float*)data;
			break;
		case HH_DENS_PARAM:
			amp->hh_dens_param = (const float*)data;
			break;
		case SWING_PARAM:
			amp->swing_param = (const float*)data;
			break;
		case CLOCK_INPUT:
			amp->clock_input = (const float*)data;
			break;
		case RESET_INPUT:
			amp->reset_input = (const float*)data;
			break;
		case MAPX_CV:
			amp->mapx_cv = (const float*)data;
			break;
		case MAPY_CV:
			amp->mapy_cv = (const float*)data;
			break;
		case CHAOS_CV:
			amp->chaos_cv = (const float*)data;
			break;
		case BD_FILL_CV:
			amp->bd_fill_cv = (const float*)data;
			break;
		case SN_FILL_CV:
			amp->sn_fill_cv = (const float*)data;
			break;
		case HH_FILL_CV:
			amp->hh_fill_cv = (const float*)data;
			break;
		case SWING_CV:
			amp->swing_cv = (const float*)data;
			break;
		case RUN_INPUT:
			amp->run_input = (const float*)data;
			break;
		case BD_OUTPUT:
			amp->bd_output = (float*)data;
			break;
		case SN_OUTPUT:
			amp->sn_output = (float*)data;
			break;
		case HH_OUTPUT:
			amp->hh_output = (float*)data;
			break;
		case BD_ACC_OUTPUT:
			amp->bd_acc_output = (float*)data;
			break;
		case SN_ACC_OUTPUT:
			amp->sn_acc_output = (float*)data;
			break;
		case HH_ACC_OUTPUT:
			amp->hh_acc_output = (float*)data;
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
	Grids* const amp = (Grids*)instance;

	const float reset_button_param = *(amp->reset_button_param);
	const float run_button_param = *(amp->run_button_param);
	const float tempo_param = *(amp->tempo_param);
	const float mapx_param = *(amp->mapx_param);
	const float mapy_param = *(amp->mapy_param);
	const float chaos_param = *(amp->chaos_param);
	const float bd_dens_param = *(amp->bd_dens_param);
	const float sn_dens_param = *(amp->sn_dens_param);
	const float hh_dens_param = *(amp->hh_dens_param);
	const float swing_param = *(amp->swing_param);

	const float* clock_input = amp->clock_input;
	const float* reset_input = amp->reset_input;
	// cv
	const float* mapx_cv = amp->mapx_cv;
	const float* mapy_cv = amp->mapy_cv;
	const float* chaos_cv = amp->chaos_cv;
	const float* bd_fill_cv = amp->bd_fill_cv;
	const float* sn_fill_cv = amp->sn_fill_cv;
	const float* hh_fill_cv = amp->hh_fill_cv;
	const float* swing_cv = amp->swing_cv;
	const float* run_input = amp->run_input;
	// out
	float* bd_output = amp->bd_output;
	float* sn_output = amp->sn_output;
	float* hh_output = amp->hh_output;
	float* bd_acc_output = amp->bd_acc_output;
	float* sn_acc_output = amp->sn_acc_output;
	float* hh_acc_output = amp->hh_acc_output;

    float* outIDs[6] = {bd_output, sn_output, hh_output,
                                 bd_acc_output, sn_acc_output, hh_acc_output};
	//
	
    float swing = 0.5;
    float swingHighTempo = 0.0;
    float swingLowTempo = 0.0;
    float mapX = 0.0f;
    float mapY = 0.0f;
    float chaos = 0.0f;
    float BDFill = 0.0;
    float SNFill = 0.0;
    float HHFill = 0.0;

    bool advStep = false;

    PatternGenerator *grids = amp->grids;

	for (uint32_t j = 0; j < n_samples; ++j) { 


		if(amp->runMode == TOGGLE) {
			if (run_button_param > 1 || run_input[j] > 1) {
				if(amp->runMode == TOGGLE){
					amp->running = !amp->running;
				}
			}
		}
		else {
			amp->running = run_button_param + run_input[j]; 
			if(amp->running <= 0) {
				amp->metro->reset();
			}
		}

		if(reset_button_param || reset_input[j]) {
			amp->grids->reset();
			amp->metro->reset();
			amp->elapsedTicks = 0;
		}

		// Clock, tempo and swing
		float tempo = clamp(tempo_param, 37.f, 240.f);
		swing = clamp(swing_param + swing_cv[j], 0.f, 0.9f);
		swingHighTempo = tempo / (1 - swing);
		swingLowTempo = tempo / (1 + swing);
		if(amp->elapsedTicks < 6) {
			amp->metro->setTempo(swingLowTempo);
		}
		else {
			amp->metro->setTempo(swingHighTempo);
		}

		// External clock select
		if(tempo_param < 38.0) {
			if(amp->initExtReset) {
				amp->grids->reset();
				amp->initExtReset = false;
			}
			amp->numTicks = ticks_granularity[amp->extClockResolution];
			amp->extClock = true;
		}
		else {
			amp->initExtReset = true;
			amp->numTicks = ticks_granularity[2];
			amp->extClock = false;
			amp->metro->process();
		}

		mapX = mapx_param + mapx_cv[j];
		mapX = clamp(mapX, 0.f, 1.f);
		mapY = mapy_param + mapy_cv[j];
		mapY = clamp(mapY, 0.f, 1.f);
		BDFill = bd_dens_param + bd_fill_cv[j];
		BDFill = clamp(BDFill, 0.f, 1.f);
		SNFill = sn_dens_param + sn_fill_cv[j];
		SNFill = clamp(SNFill, 0.f, 1.f);
		HHFill = hh_dens_param + hh_fill_cv[j];
		HHFill = clamp(HHFill, 0.f, 1.f);
		chaos = chaos_param + chaos_cv[j];
		chaos = clamp(chaos, 0.f, 1.f);

		if(amp->running) {
			if(amp->extClock) {
				if(clock_input[j] >= 1.0f) {
					advStep = true;
				}
			}
			else if(amp->metro->hasTicked()){
				advStep = true;
				amp->elapsedTicks++;
				amp->elapsedTicks %= 12;
			}
			else {
				advStep = false;
			}

			grids->setMapX((uint8_t)(mapX * 255.0));
			grids->setMapY((uint8_t)(mapY * 255.0));
			grids->setBDDensity((uint8_t)(BDFill * 255.0));
			grids->setSDDensity((uint8_t)(SNFill * 255.0));
			grids->setHHDensity((uint8_t)(HHFill * 255.0));
			grids->setRandomness((uint8_t)(chaos * 255.0));

			grids->setEuclideanLength(0, (uint8_t)(mapX * 255.0));
			grids->setEuclideanLength(1, (uint8_t)(mapY * 255.0));
			grids->setEuclideanLength(2, (uint8_t)(chaos * 255.0));
		}

		if(advStep) {
			grids->tick(amp->numTicks);
			for(int i = 0; i < 6; ++i) {
				if(grids->getDrumState(i)) {
					amp->drumTriggers[i].trigger();
					amp->gateState[i] = true;
				}
			}
			advStep = false;
		}

		if(amp->triggerOutputMode == PULSE) {
			for(int i = 0; i < 6; ++i) {
				amp->drumTriggers[i].process();
				if(amp->drumTriggers[i].getState()) {
					outIDs[i][j] = 1.0f;
				}
				else {
					outIDs[i][j] = 0.0f;
				}
			}
		}
		else if(amp->extClock && amp->triggerOutputMode == GATE) {
			for(int i = 0; i < 6; ++i) {
				if(clock_input[j] > 0 && amp->gateState[i]) {
					amp->gateState[i] = false;
					outIDs[i][j] = 1.0f;
				}
				if(clock_input[j] <= 0) {
					outIDs[i][j] = 0.0f;
				}
			}
		}
		else {
			for(int i = 0; i < 6; ++i) {
				if(amp->metro->getElapsedTickTime() < 0.5 && amp->gateState[i]) {
					outIDs[i][j] = 1.0f;
				}
				else {
					outIDs[i][j] = 0.0f;
					amp->gateState[i] = false;
				}
			}
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
	Grids* amp = (Grids*)instance;
	/* delete amp->modulator; */
	delete amp->grids;
	delete amp->metro;
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
	GRIDS_URI,
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

/* #include <iomanip> // setprecision */
/* #include <sstream> // stringstream */

/* struct Topograph : Module { */

/*     Topograph() { */
/*         /1* configParam(Topograph::TEMPO_PARAM, 0.0, 1.0, 0.406, "Tempo", "BPM", 0.f, 202.020202, 37.979797); *1/ */
/*     } */

/* }; */




// The widget

/* switch(sequencerMode) { */
/* 	case Topograph::HENRI: */
/* 		module->grids.setPatternMode(PATTERN_HENRI); */
/* 		break; */
/* 	case Topograph::OLIVIER: */
/* 		module->grids.setPatternMode(PATTERN_OLIVIER); */
/* 		break; */
/* 	case Topograph::EUCLIDEAN: */
/* 		module->grids.setPatternMode(PATTERN_EUCLIDEAN); */
/* 		module->inEuclideanMode = 1; */
/* 		break; */
/* } */


/* struct TopographAccOutputModeItem : MenuItem { */
/*     Topograph* module; */
/*     Topograph::AccOutputMode accOutputMode; */
/*     void onAction(const event::Action &e) override { */
/*         module->accOutputMode = accOutputMode; */
/*         switch(accOutputMode) { */
/*             case Topograph::INDIVIDUAL_ACCENTS: */
/*                 module->grids.setAccentAltMode(false); */
/*                 break; */
/*             case Topograph::ACC_CLK_RST: */
/*                 module->grids.setAccentAltMode(true); */
/*         } */
/*     } */
/*     void step() override { */
/*         rightText = (module->accOutputMode == accOutputMode) ? "✔" : ""; */
/*         MenuItem::step(); */
/*     } */
/* }; */

/* struct TopographClockResolutionItem : MenuItem { */
/*     Topograph* module; */
/*     Topograph::ExtClockResolution extClockResolution; */
/*     void onAction(const event::Action &e) override { */
/*         module->extClockResolution = extClockResolution; */
/*         module->grids.reset(); */
/*     } */
/*     void step() override { */
/*         rightText = (module->extClockResolution == extClockResolution) ? "✔" : ""; */
/*         MenuItem::step(); */
/*     } */
/* }; */

/* struct TopographRunModeItem : MenuItem { */
/*     Topograph* module; */
/*     Topograph::RunMode runMode; */
/*     void onAction(const event::Action &e) override { */
/*         module->runMode = runMode; */
/*     } */
/*     void step() override { */
/*         rightText = (module->runMode == runMode) ? "✔" : ""; */
/*         MenuItem::step(); */
/*     } */
/* }; */

