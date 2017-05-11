# Provides WAKAAMA_SOURCES_DIR and WAKAAMA_SOURCES and WAKAAMA_DEFINITIONS variables.
# Add LWM2M_WITH_LOGS to compile definitions to enable logging.
# Set LWM2M_LITTLE_ENDIAN to FALSE or TRUE according to your destination platform or leave
# it unset to determine endianess automatically.

ENABLE_LANGUAGE(C)
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE) 


set(WAKAAMA_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/../wakaama/core)
set(SHARED_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/../wakaama/examples/shared)
set(VSCP_DIRS ${CMAKE_CURRENT_LIST_DIR}/../vscp)
set(DEMO_DIRS ${CMAKE_CURRENT_LIST_DIR}/../examples)
set(APP_GPS_DIRS ${CMAKE_CURRENT_LIST_DIR}/../app_gps)
if(DEFINED ENV{PRJROOT})
set(RG_THREAD_HEAD $ENV{PRJROOT}/images/header/libpub/rg_thread)
set(HTTP_HEAD $ENV{PRJROOT}/images/header/oam/http)
set(RG_UPGRADE $ENV{PRJROOT}/images/header/rg_upgrade)
add_definitions(-DCONFIG_RGOS_UPGRADE)
else()
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
#SET(CMAKE_BUILD_TYPE "Debug")
set(RG_THREAD_DIRS ${CMAKE_CURRENT_LIST_DIR}/../rg_thread)
set(RG_THREAD_LIB ${CMAKE_CURRENT_LIST_DIR}/../../images/rootfs)
set(RG_THREAD_HEAD ${RG_THREAD_DIRS})

set(RG_THREAD_SOURCES
    ${RG_THREAD_DIRS}/lib.c
    ${RG_THREAD_DIRS}/lib.h
    ${RG_THREAD_DIRS}/rg_thread.c
    ${RG_THREAD_DIRS}/rg_thread.h
    ${RG_THREAD_DIRS}/timeutil.c
    ${RG_THREAD_DIRS}/timeutil.h)    

ADD_LIBRARY(rg-thread SHARED ${RG_THREAD_SOURCES})
#add_dependencies(${PROJECT_NAME} rg-thread)
LINK_DIRECTORIES(${RG_THREAD_LIB})

set(HTTP_DIRS ${CMAKE_CURRENT_LIST_DIR}/../libhttp_util)
set(HTTP_LIB ${CMAKE_CURRENT_LIST_DIR}/../../images/rootfs)
set(HTTP_HEAD ${HTTP_DIRS})

set(HTTP_SOURCES
    ${HTTP_DIRS}/http_include.h
    ${HTTP_DIRS}/http_option.c
    ${HTTP_DIRS}/http_option.h
    ${HTTP_DIRS}/http_upload.c
    ${HTTP_DIRS}/http_upload.h
    ${HTTP_DIRS}/http_util.c
	${HTTP_DIRS}/http_util.h)    

ADD_LIBRARY(http_util SHARED ${HTTP_SOURCES})
#add_dependencies(${PROJECT_NAME} rg-thread)
LINK_DIRECTORIES(${HTTP_LIB})
endif()

set(SHARED_SOURCES 
    ${SHARED_SOURCES_DIR}/platform.c
    ${SHARED_SOURCES_DIR}/memtrace.c)

if(DTLS)
    include(${CMAKE_CURRENT_LIST_DIR}/../tinydtls.cmake)

    set(SHARED_SOURCES
        ${SHARED_SOURCES}
        ${TINYDTLS_SOURCES}
        ${SHARED_SOURCES_DIR}/dtlsconnection.c)
    
    set(SHARED_INCLUDE_DIRS
        ${SHARED_SOURCES_DIR} 
        ${TINYDTLS_SOURCES_DIR})
    
    set(SHARED_DEFINITIONS -DWITH_TINYDTLS)
else()
    set(SHARED_SOURCES
        ${SHARED_SOURCES} 
        ${SHARED_SOURCES_DIR}/connection.c)

    set(SHARED_INCLUDE_DIRS ${SHARED_SOURCES_DIR})
endif() 


set(EXT_SOURCES 
    ${WAKAAMA_SOURCES_DIR}/er-coap-13/er-coap-13.c)

set(CORE_HEADERS
    ${WAKAAMA_SOURCES_DIR}/liblwm2m.h)

set(WAKAAMA_SOURCES
    ${WAKAAMA_SOURCES_DIR}/liblwm2m.c
    ${WAKAAMA_SOURCES_DIR}/uri.c
    ${WAKAAMA_SOURCES_DIR}/utils.c
    ${WAKAAMA_SOURCES_DIR}/objects.c
    ${WAKAAMA_SOURCES_DIR}/tlv.c
    ${WAKAAMA_SOURCES_DIR}/data.c
    ${WAKAAMA_SOURCES_DIR}/list.c
    ${WAKAAMA_SOURCES_DIR}/packet.c
    ${WAKAAMA_SOURCES_DIR}/transaction.c
    ${WAKAAMA_SOURCES_DIR}/registration.c
    ${WAKAAMA_SOURCES_DIR}/bootstrap.c
    ${WAKAAMA_SOURCES_DIR}/management.c
    ${WAKAAMA_SOURCES_DIR}/observe.c
    ${WAKAAMA_SOURCES_DIR}/json.c
    ${WAKAAMA_SOURCES_DIR}/discover.c
    ${CORE_HEADERS}
    ${EXT_SOURCES}
    ${SHARED_SOURCES})

# This will not work for multi project cmake generators like the Visual Studio Generator
if(CMAKE_BUILD_TYPE MATCHES Debug)
   set(WAKAAMA_DEFINITIONS ${WAKAAMA_DEFINITIONS} -DLWM2M_WITH_LOGS)
endif()

# Automatically determine endianess. This can be overwritten by setting LWM2M_LITTLE_ENDIAN
# accordingly in a cross compile toolchain file.
if(NOT DEFINED LWM2M_LITTLE_ENDIAN)
    include(TestBigEndian)
    TEST_BIG_ENDIAN(LWM2M_BIG_ENDIAN)
    if (LWM2M_BIG_ENDIAN)
         set(LWM2M_LITTLE_ENDIAN FALSE)
    else()
         set(LWM2M_LITTLE_ENDIAN TRUE)
    endif()
endif()
if (LWM2M_LITTLE_ENDIAN)
    set(WAKAAMA_DEFINITIONS ${WAKAAMA_DEFINITIONS} -DLWM2M_LITTLE_ENDIAN)
endif()
 

ADD_LIBRARY(lwm2m SHARED ${WAKAAMA_SOURCES})
SET_TARGET_PROPERTIES(lwm2m PROPERTIES LINKER_LANGUAGE C)


cmake_minimum_required (VERSION 2.0)

project (vscp)



option(DTLS "Enable DTLS" OFF)

add_definitions(-DLWM2M_CLIENT_MODE -DLWM2M_SUPPORT_JSON -DLWM2M_BIG_ENDIAN)
add_definitions(${SHARED_DEFINITIONS} ${WAKAAMA_DEFINITIONS})

include_directories (${WAKAAMA_SOURCES_DIR} ${SHARED_INCLUDE_DIRS} ${VSCP_DIRS} ${RG_THREAD_HEAD} 
                     ${HTTP_HEAD} ${RG_UPGRADE})

SET(SOURCES
    ${VSCP_DIRS}/object_security.c
    ${VSCP_DIRS}/object_server.c
    ${VSCP_DIRS}/object_device.c
    ${VSCP_DIRS}/vscp_client.h
    ${VSCP_DIRS}/vscp_client.c
    ${VSCP_DIRS}/object_firmware.c
    ${VSCP_DIRS}/object_rfid.c
    ${VSCP_DIRS}/vscp_debug.c
    )

#add_executable(${PROJECT_NAME} ${SOURCES})

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

#target_link_libraries(${PROJECT_NAME} lwm2m)
add_dependencies(${PROJECT_NAME} lwm2m)

#SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "libvscp") 

SET(DEMOCLIENT_SRC
    ${DEMO_DIRS}/vscp_demo.c
    )
add_executable(vscp_demo ${DEMOCLIENT_SRC})

SET(APP_GPS_SRC
    ${APP_GPS_DIRS}/app_gps_main.c
    ${APP_GPS_DIRS}/app_gps_server.c
    )

if(DEFINED ENV{PRJROOT})
target_link_libraries(vscp lwm2m  rg-thread http_util pthread upgrade rg-fsui cli-client rg_lsm) 
target_link_libraries(vscp_demo lwm2m vscp rg-thread rg_syslog http_util pthread upgrade rg-fsui 
                      cli-client rg_lsm)
else()
target_link_libraries(vscp lwm2m  rg-thread http_util pthread)
add_executable(app_gps_server ${APP_GPS_SRC})
target_link_libraries(vscp_demo lwm2m vscp rg-thread http_util pthread)
target_link_libraries(app_gps_server lwm2m vscp rg-thread http_util pthread)
endif()











