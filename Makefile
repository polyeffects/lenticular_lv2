SLUG = ArableInstruments
VERSION = 0.6.0

SOURCES = $(wildcard src/*.cpp) \
	eurorack/stmlib/utils/random.cc \
	eurorack/stmlib/dsp/atan.cc \
	eurorack/stmlib/dsp/units.cc \
	eurorack/clouds/dsp/correlator.cc \
	eurorack/clouds/dsp/granular_processor.cc \
	eurorack/clouds/dsp/mu_law.cc \
	eurorack/clouds/dsp/pvoc/frame_transformation.cc \
	eurorack/clouds/dsp/pvoc/phase_vocoder.cc \
	eurorack/clouds/dsp/pvoc/stft.cc \
	eurorack/clouds/resources.cc \


FLAGS += \
	-DTEST \
	-I./eurorack \
	-Wno-unused-local-typedefs

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..

include $(RACK_DIR)/plugin.mk
