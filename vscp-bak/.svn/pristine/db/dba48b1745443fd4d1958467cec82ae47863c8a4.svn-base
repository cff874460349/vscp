export top_srcdir := $(CURDIR)/..
include $(top_srcdir)/Rules.mk 

all: header build 

header:
	mkdir -p  ${EXPORT_HEADER}/viot/vscp
	cp -a vscp/vscp_client.h ${EXPORT_HEADER}/viot/vscp
	cp -a wakaama/core/liblwm2m.h ${EXPORT_HEADER}/viot/vscp
	cp -a wakaama/examples/shared/connection.h ${EXPORT_HEADER}/viot/vscp
	cp -a vscp/rfid_types.h ${EXPORT_HEADER}/viot/vscp

build:
	cd vscp && cmake . && make
	rm -rf $(install_root)
	mkdir -p $(install_sodir)
	cp -a vscp/*.so* $(install_sodir)/

.PHONY:clean
clean:
	cd vscp && make clean
	

	