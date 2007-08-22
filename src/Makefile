# .tab=4
# $Id$

TARGET = gpshook
OBJS = \
	gpshook.o \
	sceAudio_driver.o \
	sceRtc_driver.o \

BUILD_PRX=1

#USE_PSPSDK_LIBC=1
#USE_PSPSDK_LIBS=1
USE_KERNEL_LIBC=1
USE_KERNEL_LIBS=1

PRX_EXPORTS=exports.exp

INCDIR =
CFLAGS = -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

ifdef RELEASE
	CFLAGS += -DRELEASE
endif

ifdef NOLOG
	CFLAGS += -DRELEASE \
		-DNO_DEFAULT \
		-DUSE_DYN_MALLOC \
		-DUSE_STATIC_OVIS_DAT \
		-DUSE_SOUND
	
	TARGET = gpshook_nolog
endif

ifdef MINI
	CFLAGS += -DRELEASE \
		-DNO_DEFAULT \
		-DUSE_DYN_MALLOC
	
	TARGET = gpshook_mini
endif

LIBDIR =
LDFLAGS =
LIBS = -lpsppower_driver

ifdef DEBUG
	OBJS += sceHprm_driver.o debug.o
	CFLAGS += -DDEBUG
endif

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

gpshook.o: ovis_data.h

ovis_data.h: gen_ovis_dat.pl ovis0610.pot
	perl $^ $@

VERSION=093b
release:
	make allclean
	
	( \
		cd ..; \
		rm -rf gpshook[0-9][0-9][0-9]*; \
		mkdir gpshook$(VERSION); \
		cp -r src gpshook$(VERSION); \
		rm gpshook$(VERSION)/src/ovis*.pot; \
	)
	
	make RELEASE=1
	
	( \
		cd ..; \
		cp -r MS_ROOT //Mirage/yoshi/public_html/maplus_conv/gpshook.html gpshook$(VERSION); \
		cp -r src/gpshook*.prx gpshook$(VERSION)/MS_ROOT/seplugins; \
	)
	
	mkdir ../gpshook$(VERSION)/GpsLogCv
	( \
		cd ../GPSLogCv; \
		cp Release/gpslogcv.exe gpslogcv.[ch] psp.c readme.txt nme2kml.bat \
			../gpshook$(VERSION)/GpsLogCv \
	)
	
	( cd ../gpshook$(VERSION); lha a -d ../gpshook$(VERSION).lzh * )

debug:
	make DEBUG=1

allclean:
	make DEBUG=1 clean
	-rm *.prx *.elf
