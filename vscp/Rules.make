#!/bin/make
#
# This file contains rules which are shared between multiple Makefiles.
#

ifdef QUIET
Q:=@
else
Q:=
endif

CFDIR := $(shell pwd)
unexport CFDIR

#
# False targets.
#
.PHONY: dummy

#
# Special variables which should not be exported
#
unexport EXTRA_AFLAGS
unexport EXTRA_CFLAGS
unexport EXTRA_LDFLAGS
unexport EXTRA_ARFLAGS
unexport EXTRA_INCLUDE
unexport SUBDIRS
unexport SUB_DIRS
unexport EXIST_SUB_DIRS
unexport O_TARGET
unexport ADD_OBJ
unexport obj-y
unexport obj-n
unexport obj-
unexport export-objs
unexport subdir-y
unexport subdir-n
unexport subdir-
unexport obj-y-lnk

#
# Get things started.
#
first_rule: sub_dirs
ifneq ($(_PDIR), $(wildcard $(_PDIR)))
	mkdir $(_PDIR) -p
endif
	$Q$(MAKE) $(__MAKE_FLAGS__) all_targets  _PDIR="$(_PDIR)"

obj-dep := $(patsubst %.o,%.d,$(obj-y))
active-objs := $(addprefix $(_PDIR)/,$(sort $(obj-y)))
obj-y	+= $(join $(subdir-y), $(patsubst %,/_sub_%.o,$(notdir $(subdir-y))))
obj-y-lnk = $(obj-y)

SUB_DIRS	:= $(subdir-y)
EXIST_SUB_DIRS	:= $(wildcard $(subdir-y))
obj-name	:= $(patsubst %.o,%,$(obj-y))
EXTRA_LDFLAGS  = -r

#
# Common rules
#
$(_PDIR)/%.o: %.c
ifdef QUIET
	@echo "[*] Compiling `pwd`/$<"
endif
ifdef WARN
	$Q$(CC) $(CFLAGS) $(EXTRA_INCLUDE) $(EXTRA_CFLAGS) $(ADD_CFLAGS) $(CFLAGS_$(patsubst $(_PDIR)/%,%,$@)) -c  $(CFDIR)/$< -o $@
else
	$Q$(CC) $(CFLAGS) $(EXTRA_INCLUDE) $(EXTRA_CFLAGS) $(ADD_CFLAGS) $(CFLAGS_$(patsubst $(_PDIR)/%,%,$@)) -c  $< -o $@
endif
	$Q ( \
	    echo 'ifeq ($(strip $(subst $(comma),:,$(CFLAGS) $(EXTRA_INCLUDE) $(EXTRA_CFLAGS) $(ADD_CFLAGS) $(CFLAGS_$(patsubst $(_PDIR)/%,%,$@)))),$$(strip $$(subst $$(comma),:,$$(CFLAGS) $$(EXTRA_INCLUDE) $$(EXTRA_CFLAGS) $$(ADD_CFLAGS) $$(CFLAGS_$(patsubst $(_PDIR)/%,%,$@)))))' ; \
	    echo 'FILES_FLAGS_UP_TO_DATE += $@' ; \
	    echo 'endif' \
	) > $(dir $@)/.$(notdir $@).flags

$(_PDIR)/%.o: %.S
ifdef QUIET
	@echo "[*] Assembling `pwd`/$<"
endif
	$Q$(CC) $(AFLAGS) $(EXTRA_INCLUDE) $(EXTRA_AFLAGS) $(AFLAGS_$(patsubst $(_PDIR)/%,%,$@)) $< -c -o $@

$(_PDIR)/%.d: %.c
ifdef QUIET
	@echo "[*] Making depend for `pwd`/$<"
endif
	$Q$(top_srcdir)/scripts/mkdep $< "$(CFLAGS) $(EXTRA_INCLUDE) $(EXTRA_CFLAGS) $(ADD_CFLAGS) $(CFLAGS_$(patsubst $(_PDIR)/%,%,$@))" > $@

$(_PDIR)/%.d: %.S
ifdef QUIET
	@echo "[*] Making depend for `pwd`/$<"
endif
	$Q$(top_srcdir)/scripts/mkdep $< "$(AFLAGS) $(EXTRA_INCLUDE) $(EXTRA_AFLAGS) $(AFLAGS_$(patsubst $(_PDIR)/%,%,$@))" > $@

all_targets:  $(_PDIR)/$(O_TARGET)

#
# Rule to compile a set of .o files into one .o file
#
ifdef O_TARGET
$(_PDIR)/$(O_TARGET):  $(addprefix $(_PDIR)/,$(obj-dep)) $(addprefix $(_PDIR)/,$(obj-y)) 

ifneq "$(strip $(obj-y))" ""
ifdef QUIET
	@echo "[*] Linking $@"
endif
	$Q$(LD) $(EXTRA_LDFLAGS) -o $@ $(filter $(addprefix $(_PDIR)/,$(obj-y-lnk)), $^) $(ADD_OBJ)
else
ifdef QUIET 
	@echo "[*] Building library $@"
endif
	rm -f $@
	$Q$(AR) rcs $@
endif
	$Q ( \
	    echo 'ifeq ($(strip $(subst $(comma),:,$(EXTRA_LDFLAGS) $(obj-y))),$$(strip $$(subst $$(comma),:,$$(EXTRA_LDFLAGS) $$(obj-y))))' ; \
	    echo 'FILES_FLAGS_UP_TO_DATE += $@' ; \
	    echo 'endif' \
	) > $(dir $@)/.$(notdir $@).flags
endif # O_TARGET

# $(addprefix $(_PDIR)/,$(obj-y)):

# $(addprefix $(_PDIR)/,$(obj-dep)):

#
# A rule to make subdirectories
#
subdir-list = $(patsubst %,_subdir_%,$(wildcard $(SUB_DIRS)))

sub_dirs: dummy $(subdir-list)

ifdef SUB_DIRS
ifdef subdir-list
$(subdir-list) : dummy
ifneq ($(wildcard $(_PDIR)/$(patsubst _subdir_%,%,$@)),$(_PDIR)/$(patsubst _subdir_%,%,$@))
	@mkdir -p $(_PDIR)/$(patsubst _subdir_%,%,$@)
endif
	$Q$(MAKE) $(__MAKE_FLAGS__) -C $(patsubst _subdir_%,%,$@) _PDIR=$(_PDIR)/$(patsubst _subdir_%,%,$@) EXTRA_INCLUDE="$(EXTRA_INCLUDE)"
endif
endif

# 包含.d文件
FILES_DEPEND_EXIST := $(wildcard $(addprefix $(_PDIR)/,$(obj-dep)))
ifneq ($(FILES_DEPEND_EXIST),)
include $(FILES_DEPEND_EXIST)
endif

#
# A rule to do nothing
#
dummy:

#ifeq (project/$(PRODUCT_NAME)/conf/$(TARGET)/.config,$(wildcard project/$(PRODUCT_NAME)/conf/$(TARGET)/.config))
#include $(MAKEFILES)
#endif

#
# 以下是判断编译选项改动问题，如果编译选项改变，则文件重新编译
#
# Find files whose flags have changed and force recompilation.
# For safety, this works in the converse direction:
#   every file is forced, except those whose flags are positively up-to-date.
#
FILES_FLAGS_UP_TO_DATE :=

# For use in expunging commas from flags, which mung our checking.
comma = ,

FILES_FLAGS_EXIST := $(wildcard $(addprefix $(_PDIR)/,.*.flags))
ifneq ($(FILES_FLAGS_EXIST),)
include $(FILES_FLAGS_EXIST)
endif

FILES_FLAGS_CHANGED := $(strip \
    $(filter-out $(FILES_FLAGS_UP_TO_DATE), \
	$(_PDIR)/$(O_TARGET) $(active-objs)))

# A kludge: .S files don't get flag dependencies (yet),
#   because that will involve changing a lot of Makefiles.  Also
#   suppress object files explicitly listed in $(IGNORE_FLAGS_OBJS).
#   This allows handling of assembly files that get translated into
#   multiple object files (see arch/ia64/lib/idiv.S, for example).
FILES_FLAGS_CHANGED := $(strip \
    $(filter-out $(patsubst %.S, %.o, $(addprefix $(_PDIR)/,$(wildcard *.S)) $(IGNORE_FLAGS_OBJS)), \
    $(FILES_FLAGS_CHANGED)))

# 如果编译选项已改变，则重新编译
ifneq ($(FILES_FLAGS_CHANGED),)
$(FILES_FLAGS_CHANGED): dummy
endif

