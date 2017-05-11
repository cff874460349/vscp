top_srcdir := ../..
include $(top_srcdir)/Rules.mk

LIB_RG_THREAD_VER := 1.0.0
DLIB_FLAGS = -fPIC -shared
TAG := librg-thread.so
ELF := $(TAG).$(LIB_RG_THREAD_VER)
SRC := $(shell find -name '*.c')
OBJ := $(SRC:%.c=%.o)

%.o: %.c
	$(CC) -c -fPIC $(BUILD_CFLAGS) $<

.PHONY: build install clean
all: build install

build: $(OBJ)
	$(CC) $(BUILD_CFLAGS) $(DLIB_FLAGS) $^ -o $(ELF)
head:
	mkdir -p $(top_srcdir)/images/header/libpub/rg_thread
	cp ./rg_thread.h $(top_srcdir)/images/header/libpub/rg_thread
    
install:
	mkdir -p $(install_sodir)
	cp -a $(ELF) $(install_sodir)
	cd $(install_sodir) && ln -sf $(ELF) $(TAG)

clean:
	rm -rf  $(OBJ) $(ELF)
