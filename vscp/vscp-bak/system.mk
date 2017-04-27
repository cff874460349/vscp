#
# ���������Makefile
# ���ڵݹ���ø�������ģ��elf��lib��ko��Makefile����ģ�����
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
# �������Ŀ¼
#
OUTPUT_DIR := $(top_srcdir)/build

ADD_CFLAGS  =

#
# GCOV����ѡ��
#
ifeq ($(CONFIG_GCOV),y)
GCOV_FLAGS := -fprofile-arcs -ftest-coverage
endif

#
# MAKE����ѡ��
#
export MAKEFLAGS += $(JOB_SLOTS)
export MAKEFILES += ${KCONFIG}

export CONFIG_SHELL OUTPUT_DIR GCOV_FLAGS ADD_CFLAGS 
export top_srcdir  
 
unexport rootdir-y

# ���������ļ�
include $(KCONFIG)

# ���������ļ�
include $(top_srcdir)/Rules.mk

# ==================================  �޸��� =======================================

# �������������Ϊexample
PKG := viot

# �汾��Ϣ
VERSION := 1.0.0

# ���������git�ֿ⣬�����svn��Ϊgit
src_type := svn

# �������������������
# ���ﶨ����Ҫ��������Ŀ¼��Ҳ����˵��Щ��Ŀ¼�ﶼ������һ����makefile���������example���
# rootdir-$(CONFIG_EXAMPLE) += example
rootdir-y := 
rootdir-$(CFG_rg-vscp) += vscp
# ================================= �޸��� (��) ===================================

# 
# �ܵı���Ŀ�����
#
.PHONY: do-it-all
do-it-all: build

#
# ����
# ���ܲ���forѭ�����еݹ�ʽ���ã�����ﲻ�����б���Ч��
#
.PHONY: build
build: $(patsubst %, _dir_%, $(rootdir-y))
$(patsubst %, _dir_%, $(rootdir-y)): 
	$(MAKE) CFLAGS="$(CFLAGS)" -C $(top_srcdir)/$(patsubst _dir_%,%, $@) _PDIR="$(OUTPUT_DIR)/$(patsubst _dir_%,%, $@)"
	@echo "Compiled OK! find output in $(OUTPUT_DIR)/$(patsubst _dir_%,%, $@)"
	@echo

#
# ��װ
#	
.PHONY: install
install:
	n=`ls $(install_root)/ | wc -l`; \
	if [ $$n -gt 0 ] ; then \
		cp -a -rf $(install_root)/* ${IMAGES}; fi

#
# ���
# example.spec�ļ�����ǰ׺example�ǲֿ����ƣ�����ֿ���Ϊlibpub�������ȡ��Ϊlibpub.spec����Ӧ���ڵ�ǰĿ¼�£���libpub.spec�ļ���
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
# �汾��Ϣ
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
