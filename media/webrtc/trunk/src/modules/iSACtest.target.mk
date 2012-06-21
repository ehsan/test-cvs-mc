# This file is generated by gyp; do not edit.

TOOLSET := target
TARGET := iSACtest
DEFS_Debug := '-D_FILE_OFFSET_BITS=64' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_NSS=1' \
	'-DTOOLKIT_USES_GTK=1' \
	'-DGTK_DISABLE_SINGLE_INCLUDES=1' \
	'-DENABLE_REMOTING=1' \
	'-DENABLE_P2P_APIS=1' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DENABLE_INPUT_SPEECH' \
	'-DENABLE_NOTIFICATIONS' \
	'-DENABLE_GPU=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DUSE_SKIA=1' \
	'-DENABLE_REGISTER_PROTOCOL_HANDLER=1' \
	'-DENABLE_WEB_INTENTS=1' \
	'-DENABLE_PLUGIN_INSTALLATION=1' \
	'-DWEBRTC_TARGET_PC' \
	'-DWEBRTC_LINUX' \
	'-DWEBRTC_THREAD_RR' \
	'-D__STDC_FORMAT_MACROS' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=1' \
	'-DWTF_USE_DYNAMIC_ANNOTATIONS=1' \
	'-D_DEBUG'

# Flags passed to all source files.
CFLAGS_Debug := -Werror \
	-pthread \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-O0 \
	-g

# Flags passed to only C files.
CFLAGS_C_Debug := 

# Flags passed to only C++ files.
CFLAGS_CC_Debug := -fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare

INCS_Debug := -Isrc \
	-I. \
	-Isrc/modules/audio_coding/codecs/iSAC/main/test \
	-Isrc/modules/audio_coding/codecs/iSAC/main/interface \
	-Isrc/modules/audio_coding/codecs/iSAC/main/util

DEFS_Release := '-D_FILE_OFFSET_BITS=64' \
	'-DCHROMIUM_BUILD' \
	'-DUSE_NSS=1' \
	'-DTOOLKIT_USES_GTK=1' \
	'-DGTK_DISABLE_SINGLE_INCLUDES=1' \
	'-DENABLE_REMOTING=1' \
	'-DENABLE_P2P_APIS=1' \
	'-DENABLE_CONFIGURATION_POLICY' \
	'-DENABLE_INPUT_SPEECH' \
	'-DENABLE_NOTIFICATIONS' \
	'-DENABLE_GPU=1' \
	'-DENABLE_EGLIMAGE=1' \
	'-DUSE_SKIA=1' \
	'-DENABLE_REGISTER_PROTOCOL_HANDLER=1' \
	'-DENABLE_WEB_INTENTS=1' \
	'-DENABLE_PLUGIN_INSTALLATION=1' \
	'-DWEBRTC_TARGET_PC' \
	'-DWEBRTC_LINUX' \
	'-DWEBRTC_THREAD_RR' \
	'-D__STDC_FORMAT_MACROS' \
	'-DNDEBUG' \
	'-DNVALGRIND' \
	'-DDYNAMIC_ANNOTATIONS_ENABLED=0'

# Flags passed to all source files.
CFLAGS_Release := -Werror \
	-pthread \
	-fno-exceptions \
	-fno-strict-aliasing \
	-Wall \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-fvisibility=hidden \
	-pipe \
	-fPIC \
	-Wextra \
	-Wno-unused-parameter \
	-Wno-missing-field-initializers \
	-O2 \
	-fno-ident \
	-fdata-sections \
	-ffunction-sections

# Flags passed to only C files.
CFLAGS_C_Release := 

# Flags passed to only C++ files.
CFLAGS_CC_Release := -fno-rtti \
	-fno-threadsafe-statics \
	-fvisibility-inlines-hidden \
	-Wsign-compare

INCS_Release := -Isrc \
	-I. \
	-Isrc/modules/audio_coding/codecs/iSAC/main/test \
	-Isrc/modules/audio_coding/codecs/iSAC/main/interface \
	-Isrc/modules/audio_coding/codecs/iSAC/main/util

OBJS := $(obj).target/$(TARGET)/src/modules/audio_coding/codecs/iSAC/main/test/simpleKenny.o \
	$(obj).target/$(TARGET)/src/modules/audio_coding/codecs/iSAC/main/util/utility.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# Make sure our dependencies are built before any of us.
$(OBJS): | $(obj).target/src/modules/libiSAC.a $(obj).target/src/common_audio/libsignal_processing.a

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.c FORCE_DO_CMD
	@$(call do_cmd,cc,1)

# End of this set of suffix rules
### Rules for final target.
LDFLAGS_Debug := -pthread \
	-Wl,-z,noexecstack \
	-fPIC \
	-B$(builddir)/../../third_party/gold

LDFLAGS_Release := -pthread \
	-Wl,-z,noexecstack \
	-fPIC \
	-B$(builddir)/../../third_party/gold \
	-Wl,-O1 \
	-Wl,--as-needed \
	-Wl,--gc-sections

LIBS := 

$(builddir)/iSACtest: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(builddir)/iSACtest: LIBS := $(LIBS)
$(builddir)/iSACtest: LD_INPUTS := $(OBJS) $(obj).target/src/modules/libiSAC.a $(obj).target/src/common_audio/libsignal_processing.a
$(builddir)/iSACtest: TOOLSET := $(TOOLSET)
$(builddir)/iSACtest: $(OBJS) $(obj).target/src/modules/libiSAC.a $(obj).target/src/common_audio/libsignal_processing.a FORCE_DO_CMD
	$(call do_cmd,link)

all_deps += $(builddir)/iSACtest
# Add target alias
.PHONY: iSACtest
iSACtest: $(builddir)/iSACtest

