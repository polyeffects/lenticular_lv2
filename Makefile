SOURCES = $(wildcard src/*.cpp) \
	parasites/stmlib/utils/random.cc \
	parasites/stmlib/dsp/atan.cc \
	parasites/stmlib/dsp/units.cc \
	parasites/clouds/dsp/correlator.cc \
	parasites/clouds/dsp/granular_processor.cc \
	parasites/clouds/dsp/mu_law.cc \
	parasites/clouds/dsp/pvoc/frame_transformation.cc \
	parasites/clouds/dsp/pvoc/phase_vocoder.cc \
	parasites/clouds/dsp/pvoc/stft.cc \
	parasites/clouds/resources.cc

FLAGS += \
	-DTEST \
	-DPARASITES \
	-I./parasites \
	-Wno-unused-local-typedefs

DISTRIBUTABLES += $(wildcard LICENSE*) res

RACK_DIR ?= ../..

include $(RACK_DIR)/plugin.mk
