#
# makefile
# Mircobit Makefile
#

.PHONY: all install remove clean
.DEFAULT: all

DIR:=$(shell yt --plain target | head -n 1 | cut -f 1 -d' ')
TARGET:=$(shell yt --plain ls | head -n 1 | cut -f 1 -d' ')

indicator: indicator.cpp
	cp $< source/main.cpp
	yt target bbc-microbit-classic-gcc
	yt build

signaler: signaler.cpp
	cp $< source/main.cpp
	yt target bbc-microbit-classic-gcc
	yt build

sinstall: 
	cp build/$(DIR)/source/$(TARGET)-combined.hex  "/Volumes/MICROBIT 1"

install: 
	cp build/$(DIR)/source/$(TARGET)-combined.hex  /Volumes/MICROBIT

clean:
	yt clean

remove:
	rm /Volumes/MICROBIT/$(TARGET)-combined.hex
