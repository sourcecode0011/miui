PROJECT_DIR := $(shell pwd)
CC = ../../prebuilt/gcc/linux-x86/arm/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-gcc
CXX = ../../prebuilt/gcc/linux-x86/arm/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-g++

BIN = ui

STAGING_DIR = /home/developer/luwenjiang/databack/v306/v306/out/v306-h300/staging_dir/target/
SYSROOT = /home/luwenjiang/3128linux/buildroot/output/rockchip_rk3128_game/host/arm-buildroot-linux-gnueabihf/sysroot/


#OBJ =     main.o\

OBJ =     start.o\
		png.o \
		jpeg.o \
		fb_display.o \
		rthreads/rthreads.o \
		playpcm.o \
		testmode.o \
		processdisp.o \
		testcolor.o \
		textdraw.o \
		graphics.o \
		resources.o

#OBJ =     testdisp.o\
	disp.o
	
			

#-DHAVE_OVERLAY  -DHAVE_WAYLAND -DHAVE_KMS  -DHAVE_THREADS HAVE_EGL -DPIC -DHAVE_ENDIAN_H
CFLAGS =  -I. -DMODULE_DISP_COMPILE_CFG=1 -DOVERSCAN_PERCENT=0 -DDEFAULT_ROTATION=ROTATION_LEFT\
		-I./include \
	  -I$(STAGING_DIR)/usr/include \
	  -L$(STAGING_DIR)/usr/lib -L./ -ljpeg -lpng -lz -lasound \
#	  -lstdc++\
	    
#-lminui
$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) 
	
%.o: %.cpp
	$(CXX) -c $< -o $@ $(CFLAGS) 

clean:
	rm -rf $(OBJ) $(BIN)
