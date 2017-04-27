#
# 组件包的总Makefile
# 用于递归调用各个进程模块elf、lib、ko的Makefile进行模块编译
#
CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)
CENV := $(shell if [ `uname` = "Linux" ]; then echo linux; else echo cygwin; fi)

#
# Top directory definition.
#
top_srcdir := $(shell pwd)

#
# ALL target.
#
all: do-it-all

#
# 编译输出目录
#
OUTPUT_DIR := $(top_srcdir)/build

ADD_CFLAGS  =

#
# GCOV编译选项
#
ifeq ($(CONFIG_GCOV),y)
GCOV_FLAGS := -fprofile-arcs -ftest-coverage
endif

#
# MAKE编译选项
#
export MAKEFLAGS += $(JOB_SLOTS)
export MAKEFILES += ${KCONFIG}

export CONFIG_SHELL OUTPUT_DIR GCOV_FLAGS ADD_CFLAGS 
export top_srcdir  
 
unexport rootdir-y

# 包含配置文件
include $(KCONFIG)

# 包含规则文件
include $(top_srcdir)/Rules.mk

# ==================================  修改区 =======================================

# 例如组件包名称为example
PKG := viot

# 版本信息
VERSION := 1.0.0

# 如果代码是git仓库，则需把svn改为git
src_type := svn

# 添加组件包包含的组件，
# 这里定义需要遍历的子目录，也就是说这些子目录里都含有下一级的makefile。例如包含example组件
# rootdir-$(CONFIG_EXAMPLE) += example
rootdir-y := 
rootdir-$(CFG_rg-vscp) += vscp
# ================================= 修改区 (完) ===================================

# 
# 总的编译目标入口
#
.PHONY: do-it-all
do-it-all: build

#
# 编译
# 不能采用for循环进行递归式调用，否则达不到并行编译效果
#
.PHONY: build
build: $(patsubst %, _dir_%, $(rootdir-y))
$(patsubst %, _dir_%, $(rootdir-y)): 
	$(MAKE) CFLAGS="$(CFLAGS)" -C $(top_srcdir)/$(patsubst _dir_%,%, $@) _PDIR="$(OUTPUT_DIR)/$(patsubst _dir_%,%, $@)"
	@echo "Compiled OK! find output in $(OUTPUT_DIR)/$(patsubst _dir_%,%, $@)"
	@echo

#
# 安装
#	
.PHONY: install
install:
	n=`ls $(install_root)/ | wc -l`; \
	if [ $$n -gt 0 ] ; then \
		cp -a -rf $(install_root)/* ${IMAGES}; fi

#
# 打包
# example.spec文件名的前缀example是仓库名称，如果仓库名为libpub，则可以取名为libpub.spec，相应的在当前目录下，有libpub.spec文件。
#
.PHONY: rpmpkg
rpmpkg:
	$(RPMPKG) $(install_rootfs_dir) $(PKG).spec $(src_type)

#
# gcov
#	
.PHONY: gcov-docs
gcov-docs:
	@-for dir in $(rootdir-y) ; do \
	lcov -c -b $$dir -d $$dir -o $$dir.info; \
	genhtml -o $(GCOV_DOCS)/$(PKG)-$(VERSION)/$$dir $$dir.info --title $$dir; done

#
# 版本信息
#	
.PHONY: version
version:
	@echo ${VERSION}

.PHONY:gcov-cp-gcda
gcov-cp-gcda:
	@tar zxf gcov_*.tar.gz -C $(PKG)
	
.PHONY: clean
clean:
	@-for dir in $(rootdir-y) ; do \
	$(MAKE) -C $$dir clean ; done
	@rm -rf $(shell find -name "*.gcda" && find -name "*.gcno" && find -name "report" && find -name "*.info" && find -name "*.o")
	@rm -rf $(install_root)
	@rm -rf $(OUTPUT_DIR) 
