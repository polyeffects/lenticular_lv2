#!/usr/bin/make -f
OPTIMIZATIONS ?= -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -DNDEBUG -fexpensive-optimizations  -ftree-vectorize -funroll-loops -fvariable-expansion-in-unroller -funsafe-loop-optimizations -ftree-loop-im -funswitch-loops -ftree-loop-ivcanon -ftracer -fprefetch-loop-arrays -freorder-blocks-and-partition -funsafe-math-optimizations -ffinite-math-only -fdata-sections -openmp-simd  -g
# -fno-exceptions 
#-combine
# OPTIMIZATIONS ?= -mtune=cortex-a53 -funsafe-math-optimizations -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -fvisibility=hidden -fdata-sections -ffunction-sections  -DNDEBUG # -fopt-info-vec-optimize
# OPTIMIZATIONS = -msse
PREFIX ?= /usr

# DEBUGFLAGS = -D_FORTIFY_SOURCE=2 -O0 -g -Wl,-z,relro,-z,now -fPIC -DPIC -Wall -D DEBUG -D NOSSE
# DEBUGFLAGS = -D DEBUG -D_FORTIFY_SOURCE=2 -O3 -g -Wl,-z,relro,-z,now -fPIC -DPIC -Wall -D DEBUG #-D NOSSE
CFLAGS ?= -fPIC $(OPTIMIZATIONS) # $(DEBUGFLAGS)
# CFLAGS ?= -fPIC $(DEBUGFLAGS)

PKG_CONFIG?=pkg-config
STRIP?=strip
STRIPFLAGS?=-s

polyclouds_VERSION?=$(shell git describe --tags HEAD 2>/dev/null | sed 's/-g.*$$//;s/^v//' || echo "LV2")
###############################################################################

LV2DIR ?= $(PREFIX)/lib/lv2
LOADLIBES=-lm
LV2NAME_CLOUDS=polyclouds
LV2NAME_WARPS=polywarps
LV2NAME_PLAITS=polyplaits
LV2NAME_GRIDS=polygrids
LV2NAME_MARBLES=polymarbles
LV2NAME_RINGS=polyrings
LV2NAME_TIDES=polytides
BUNDLE=polylenticular.lv2
BUILDDIR=build/
targets=

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  LV2LDFLAGS=-dynamiclib
  LIB_EXT=.dylib
  EXTENDED_RE=-E
  STRIPFLAGS=-u -r -arch all -s lv2syms
  targets+=lv2syms
else
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic
  LIB_EXT=.so
  EXTENDED_RE=-r
endif

ifneq ($(XWIN),)
  CC=$(XWIN)-gcc
  STRIP=$(XWIN)-strip
  LV2LDFLAGS=-Wl,-Bstatic -Wl,-Bdynamic -Wl,--as-needed
  LIB_EXT=.dll
  override LDFLAGS += -static-libgcc -static-libstdc++
else
  override CFLAGS += -fPIC -fvisibility=hidden
endif

targets+=$(BUILDDIR)$(LV2NAME_CLOUDS)$(LIB_EXT)
targets+=$(BUILDDIR)$(LV2NAME_WARPS)$(LIB_EXT)
targets+=$(BUILDDIR)$(LV2NAME_PLAITS)$(LIB_EXT)
targets+=$(BUILDDIR)$(LV2NAME_GRIDS)$(LIB_EXT)
targets+=$(BUILDDIR)$(LV2NAME_MARBLES)$(LIB_EXT)
targets+=$(BUILDDIR)$(LV2NAME_RINGS)$(LIB_EXT)
targets+=$(BUILDDIR)$(LV2NAME_TIDES)$(LIB_EXT)

###############################################################################
# extract versions
LV2VERSION=1

# check for build-dependencies
ifeq ($(shell $(PKG_CONFIG) --exists lv2 || echo no), no)
  $(error "LV2 SDK was not found")
endif

CLOUDS_SOURCES = src/polyclouds.cpp \
	parasites/stmlib/utils/random.cc \
	parasites/stmlib/dsp/atan.cc \
	parasites/stmlib/dsp/units.cc \
	parasites/clouds/dsp/correlator.cc \
	parasites/clouds/dsp/granular_processor.cc \
	parasites/clouds/dsp/mu_law.cc \
	parasites/clouds/dsp/pvoc/frame_transformation.cc \
	parasites/clouds/dsp/pvoc/phase_vocoder.cc \
	parasites/clouds/dsp/pvoc/stft.cc \
 	parasites/clouds/dsp/kammerl_player.cc \
	parasites/clouds/resources.cc

WARPS_SOURCES = src/polywarps.cpp \
	parasites/warps/dsp/modulator.cc \
	parasites/warps/dsp/oscillator.cc \
	parasites/warps/dsp/vocoder.cc \
	parasites/warps/dsp/filter_bank.cc \
	parasites/warps/resources.cc  \
	parasites/stmlib/utils/random.cc \
	parasites/stmlib/dsp/atan.cc \
	parasites/stmlib/dsp/units.cc 

GRIDS_SOURCES = src/polygrids.cpp \
	src/Metronome.cpp  \
	src/TopographPatternGenerator.cpp \
	src/Oneshot.cpp

MARBLES_SOURCES = src/polymarbles.cpp 
MARBLES_SOURCES += eurorack/stmlib/utils/random.cc
MARBLES_SOURCES += eurorack/stmlib/dsp/atan.cc
MARBLES_SOURCES += eurorack/stmlib/dsp/units.cc
MARBLES_SOURCES += eurorack/marbles/random/t_generator.cc
MARBLES_SOURCES += eurorack/marbles/random/x_y_generator.cc
MARBLES_SOURCES += eurorack/marbles/random/output_channel.cc
MARBLES_SOURCES += eurorack/marbles/random/lag_processor.cc
MARBLES_SOURCES += eurorack/marbles/random/quantizer.cc
MARBLES_SOURCES += eurorack/marbles/ramp/ramp_extractor.cc
MARBLES_SOURCES += eurorack/marbles/resources.cc

PLAITS_SOURCES = src/polyplaits.cpp 
PLAITS_SOURCES += $(wildcard eurorack/plaits/dsp/*.cc)
PLAITS_SOURCES += $(wildcard eurorack/plaits/dsp/engine/*.cc)
PLAITS_SOURCES += $(wildcard eurorack/plaits/dsp/speech/*.cc)
PLAITS_SOURCES += $(wildcard eurorack/plaits/dsp/physical_modelling/*.cc)
PLAITS_SOURCES += eurorack/plaits/resources.cc
PLAITS_SOURCES += eurorack/stmlib/utils/random.cc
PLAITS_SOURCES += eurorack/stmlib/dsp/atan.cc
PLAITS_SOURCES += eurorack/stmlib/dsp/units.cc


RINGS_SOURCES = src/polyrings.cpp 
RINGS_SOURCES += eurorack/rings/dsp/fm_voice.cc
RINGS_SOURCES += eurorack/rings/dsp/part.cc
RINGS_SOURCES += eurorack/rings/dsp/string_synth_part.cc
RINGS_SOURCES += eurorack/rings/dsp/string.cc
RINGS_SOURCES += eurorack/rings/dsp/resonator.cc
RINGS_SOURCES += eurorack/rings/resources.cc
RINGS_SOURCES += eurorack/stmlib/utils/random.cc
RINGS_SOURCES += eurorack/stmlib/dsp/atan.cc
RINGS_SOURCES += eurorack/stmlib/dsp/units.cc

TIDES_SOURCES = src/polytides.cpp 
TIDES_SOURCES += eurorack/tides2/poly_slope_generator.cc
TIDES_SOURCES += eurorack/tides2/ramp_extractor.cc
TIDES_SOURCES += eurorack/tides2/resources.cc
TIDES_SOURCES += eurorack/stmlib/dsp/units.cc

FLAGS += \
	-DTEST \
	-DPARASITES \
	-I./parasites \
	-Wno-unused-local-typedefs

MUT_FLAGS += \
	-DTEST \
	-I./eurorack \
	-Wno-unused-local-typedefs

# override CFLAGS += -std=c99 `$(PKG_CONFIG) --cflags lv2`
override CFLAGS += `$(PKG_CONFIG) --cflags lv2`

# build target definitions
default: all

all: $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME_WARPS).ttl $(BUILDDIR)$(LV2NAME_CLOUDS).ttl $(BUILDDIR)$(LV2NAME_PLAITS).ttl $(BUILDDIR)$(LV2NAME_GRIDS).ttl $(BUILDDIR)$(LV2NAME_MARBLES).ttl $(BUILDDIR)$(LV2NAME_RINGS).ttl $(BUILDDIR)$(LV2NAME_TIDES).ttl $(targets)

lv2syms:
	echo "_lv2_descriptor" > lv2syms

$(BUILDDIR)manifest.ttl: manifest.ttl
	@mkdir -p $(BUILDDIR)
	cat manifest.ttl > $(BUILDDIR)manifest.ttl

$(BUILDDIR)$(LV2NAME_CLOUDS).ttl: $(LV2NAME_CLOUDS).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_CLOUDS).ttl > $(BUILDDIR)$(LV2NAME_CLOUDS).ttl

$(BUILDDIR)$(LV2NAME_WARPS).ttl: $(LV2NAME_WARPS).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_WARPS).ttl > $(BUILDDIR)$(LV2NAME_WARPS).ttl

$(BUILDDIR)$(LV2NAME_PLAITS).ttl: $(LV2NAME_PLAITS).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_PLAITS).ttl > $(BUILDDIR)$(LV2NAME_PLAITS).ttl

$(BUILDDIR)$(LV2NAME_GRIDS).ttl: $(LV2NAME_GRIDS).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_GRIDS).ttl > $(BUILDDIR)$(LV2NAME_GRIDS).ttl

$(BUILDDIR)$(LV2NAME_MARBLES).ttl: $(LV2NAME_MARBLES).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_MARBLES).ttl > $(BUILDDIR)$(LV2NAME_MARBLES).ttl

$(BUILDDIR)$(LV2NAME_RINGS).ttl: $(LV2NAME_RINGS).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_RINGS).ttl > $(BUILDDIR)$(LV2NAME_RINGS).ttl
	""
$(BUILDDIR)$(LV2NAME_TIDES).ttl: $(LV2NAME_TIDES).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME_TIDES).ttl > $(BUILDDIR)$(LV2NAME_TIDES).ttl

$(BUILDDIR)$(LV2NAME_CLOUDS)$(LIB_EXT): $(CLOUDS_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(FLAGS) \
	  -o $(BUILDDIR)$(LV2NAME_CLOUDS)$(LIB_EXT) $(CLOUDS_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)
	#$(STRIP) $(STRIPFLAGS) $(BUILDDIR)$(LV2NAME_CLOUDS)$(LIB_EXT)

$(BUILDDIR)$(LV2NAME_WARPS)$(LIB_EXT): $(WARPS_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(FLAGS) \
	  -o $(BUILDDIR)$(LV2NAME_WARPS)$(LIB_EXT) $(WARPS_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

$(BUILDDIR)$(LV2NAME_PLAITS)$(LIB_EXT): $(PLAITS_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(MUT_FLAGS) \
	  -o $(BUILDDIR)$(LV2NAME_PLAITS)$(LIB_EXT) $(PLAITS_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

$(BUILDDIR)$(LV2NAME_GRIDS)$(LIB_EXT): $(GRIDS_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) \
	  -o $(BUILDDIR)$(LV2NAME_GRIDS)$(LIB_EXT) $(GRIDS_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

$(BUILDDIR)$(LV2NAME_MARBLES)$(LIB_EXT): $(MARBLES_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(MUT_FLAGS) -fPIC \
	  -o $(BUILDDIR)$(LV2NAME_MARBLES)$(LIB_EXT) $(MARBLES_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

$(BUILDDIR)$(LV2NAME_RINGS)$(LIB_EXT): $(RINGS_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(MUT_FLAGS) -fPIC \
	  -o $(BUILDDIR)$(LV2NAME_RINGS)$(LIB_EXT) $(RINGS_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

$(BUILDDIR)$(LV2NAME_TIDES)$(LIB_EXT): $(TIDES_SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(MUT_FLAGS) -fPIC \
	  -o $(BUILDDIR)$(LV2NAME_TIDES)$(LIB_EXT) $(TIDES_SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)

# install/uninstall/clean target definitions

install: all
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_CLOUDS)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_WARPS)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_PLAITS)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_GRIDS)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_MARBLES)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_RINGS)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME_TIDES)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m644 $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME_CLOUDS).ttl $(BUILDDIR)$(LV2NAME_WARPS).ttl $(BUILDDIR)$(LV2NAME_PLAITS).ttl $(BUILDDIR)$(LV2NAME_GRIDS).ttl $(BUILDDIR)$(LV2NAME_MARBLES).ttl $(BUILDDIR)$(LV2NAME_RINGS).ttl $(BUILDDIR)$(LV2NAME_TIDES).ttl $(DESTDIR)$(LV2DIR)/$(BUNDLE)

uninstall:
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/manifest.ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_CLOUDS).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_CLOUDS)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_WARPS).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_WARPS)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_PLAITS).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_PLAITS)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_GRIDS).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_GRIDS)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_MARBLES).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_MARBLES)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_RINGS).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_RINGS)$(LIB_EXT)
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_TIDES).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME_TIDES)$(LIB_EXT)
	-rmdir $(DESTDIR)$(LV2DIR)/$(BUNDLE)

$(LV2NAME_CLOUDS)debug : clean $(RES_OBJECTS)
	$(CXX) $(DEBUGFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).so
	$(CC) $(DEBUGFLAGS) -Wl,-z,nodelete $(GUI_OBJECTS) $(RES_OBJECTS) $(GUI_LDFLAGS) -o $(NAME)_ui.so

clean:
	rm -f $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME_CLOUDS).ttl $(BUILDDIR)$(LV2NAME_CLOUDS)$(LIB_EXT) lv2syms
	-test -d $(BUILDDIR) && rmdir $(BUILDDIR) || true

.PHONY: clean all install uninstall
