#!/usr/bin/make -f
OPTIMIZATIONS ?= -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -DNDEBUG
# OPTIMIZATIONS = -msse
# OPTIMIZATIONS ?= -mtune=cortex-a53 -funsafe-math-optimizations -ffast-math -fomit-frame-pointer -O3 -fno-finite-math-only -fvisibility=hidden -fdata-sections -ffunction-sections  -DNDEBUG # -fopt-info-vec-optimize
PREFIX ?= /usr

DEBUGFLAGS = -D_FORTIFY_SOURCE=2 -Wl,-z,relro,-z,now -fPIC -DPIC -Wall -D DEBUG -D NOSSE
CFLAGS ?= $(OPTIMIZATIONS) -Wall #-g $(DEBUGFLAGS)

PKG_CONFIG?=pkg-config
STRIP?=strip
STRIPFLAGS?=-s

polyclouds_VERSION?=$(shell git describe --tags HEAD 2>/dev/null | sed 's/-g.*$$//;s/^v//' || echo "LV2")
###############################################################################

LV2DIR ?= $(PREFIX)/lib/lv2
LOADLIBES=-lm
LV2NAME=polyclouds
BUNDLE=polyclouds.lv2
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

targets+=$(BUILDDIR)$(LV2NAME)$(LIB_EXT)

###############################################################################
# extract versions
LV2VERSION=1

# check for build-dependencies
ifeq ($(shell $(PKG_CONFIG) --exists lv2 || echo no), no)
  $(error "LV2 SDK was not found")
endif

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

# override CFLAGS += -std=c99 `$(PKG_CONFIG) --cflags lv2`
override CFLAGS += `$(PKG_CONFIG) --cflags lv2`

# build target definitions
default: all

all: $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME).ttl $(targets)

lv2syms:
	echo "_lv2_descriptor" > lv2syms

$(BUILDDIR)manifest.ttl: manifest.ttl
	@mkdir -p $(BUILDDIR)
	cat manifest.ttl > $(BUILDDIR)manifest.ttl

$(BUILDDIR)$(LV2NAME).ttl: $(LV2NAME).ttl
	@mkdir -p $(BUILDDIR)
	cat $(LV2NAME).ttl > $(BUILDDIR)$(LV2NAME).ttl

$(BUILDDIR)$(LV2NAME)$(LIB_EXT): $(SOURCES) 
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(FLAGS) \
	  -o $(BUILDDIR)$(LV2NAME)$(LIB_EXT) $(SOURCES) \
		-shared $(LV2LDFLAGS) $(LDFLAGS) $(LOADLIBES)
	$(STRIP) $(STRIPFLAGS) $(BUILDDIR)$(LV2NAME)$(LIB_EXT)

# install/uninstall/clean target definitions

install: all
	install -d $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m755 $(BUILDDIR)$(LV2NAME)$(LIB_EXT) $(DESTDIR)$(LV2DIR)/$(BUNDLE)
	install -m644 $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME).ttl $(DESTDIR)$(LV2DIR)/$(BUNDLE)

uninstall:
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/manifest.ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME).ttl
	rm -f $(DESTDIR)$(LV2DIR)/$(BUNDLE)/$(LV2NAME)$(LIB_EXT)
	-rmdir $(DESTDIR)$(LV2DIR)/$(BUNDLE)

$(LV2NAME)debug : clean $(RES_OBJECTS)
	$(CXX) $(DEBUGFLAGS) $(OBJECTS) $(LDFLAGS) -o $(NAME).so
	$(CC) $(DEBUGFLAGS) -Wl,-z,nodelete $(GUI_OBJECTS) $(RES_OBJECTS) $(GUI_LDFLAGS) -o $(NAME)_ui.so

clean:
	rm -f $(BUILDDIR)manifest.ttl $(BUILDDIR)$(LV2NAME).ttl $(BUILDDIR)$(LV2NAME)$(LIB_EXT) lv2syms
	-test -d $(BUILDDIR) && rmdir $(BUILDDIR) || true

.PHONY: clean all install uninstall
