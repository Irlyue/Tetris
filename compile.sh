#!/bin/bash

g++ main.cpp \
	-I/usr/include/SDL2 \
	-Wall \
	-lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer \
	-o tetris
