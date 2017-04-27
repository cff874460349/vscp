CC ?= $(CROSS)gcc
AR ?= $(CROSS)ar
AS ?= $(CROSS)as
LD ?= $(CROSS)ld
STRIP ?= $(CROSS)strip

ifndef top_srcdir
top_srcdir:=$(CURDIR)
endif
# top_srcdir是lsm仓库的顶层目录路径；
# install_root是仓库里的安装目录树
install_root:=${top_srcdir}/images

install_header_dir:=${install_root}/header
install_adir:=$(install_root)/lib
install_symbol_dir:=$(install_root)/symbol
# symbol_files是内核模块符号表
symbol_files:=$(shell find $(EXPORT_SYMBOL) -name 'Module.symvers')
#
# symbol_files += $(shell find $(install_symbol_dir) -name 'Module.symvers')
# 例如ﺿ内核模块B，如果调用了内核模块A（模块A的符号表已导出）的函数，则可在模块B的makefile文件中，声明符号表的位置，使用以下两行语句：
# KBUILD_EXTRA_SYMBOLS += $(symbol_files)
# export KBUILD_EXTRA_SYMBOLS

# top root: install_rootfs_dir
# install_rootfs_dir是rootfs根文件系统的安装目录树images/rootfs
install_rootfs_dir:=$(install_root)/rootfs

# install_sodir是动态安装路庿 仓库里的组件编译安装so时，makefile的install目标可执行命令cp xx.so $(install_sodir)
install_sodir:=$(install_rootfs_dir)/$(INSTALL_SODIR)

install_usr_bin_dir:=$(install_rootfs_dir)/usr/bin
# install_sbin_dir是系统应用程序的安装路径
install_sbin_dir:=$(install_rootfs_dir)/sbin
install_etc_dir:=$(install_rootfs_dir)/etc
export INSTALL_MOD_PATH:=$(ROOT)

# 内核模块的安装路弿export INSTALL_MOD_PATH:=$(ROOT)

# 可以使用BUILD_CFLAGS参数，添加模块目录下自己的头文件搜索路径
# 可以使用BUILD_LDFLAGS参数，添加模块目录下本仓库内其它组件编译后生成的动态库搜索路径
BUILD_CFLAGS:= -I$(install_header_dir)
BUILD_LDFLAGS:=$(LDFLAGS) -L/$(install_sodir) -L/$(install_adir)

ifeq ("$(PRJRELEASE)","gcov")
BUILD_CFLAGS += -fprofile-arcs -ftest-coverage
endif

ifeq ("$(PRJRELEASE)","gcov")
LINKFLAGS += -fprofile-arcs -ftest-coverage
endif

define compile_dirs
.PHONY: $(1)
$(1):
	@echo;echo "building $(1)..."
	@$(MAKE) -C ${1}
endef

#include $(top_srcdir)/def.config
# 包含工程里所有组件的kconfig文件中，定义的宏开僿include $(KCONFIG)
