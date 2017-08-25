# Makefile for Basler Pylon sample program
.PHONY			: all clean install

# The program to build
NAME			:= grabzbar

# Installation directories for GenICam and Pylon
PYLON_ROOT		?= /opt/pylon5
GENICAM_ROOT	?= $(PYLON_ROOT)/genicam

# Build tools and flags
CXX				?= g++

LD         := $(CXX)
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags) $(shell pkg-config zbar --cflags) $(shell pkg-config opencv --cflags)
CXXFLAGS   := -std=c++11 #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs) $(shell pkg-config zbar --cflags --libs) $(shell pkg-config opencv --libs) -lboost_system -lboost_regex -lboost_filesystem -lboost_thread

# Rules for building
all				: $(NAME)

$(NAME)			: $(NAME).o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(NAME).o: src/$(NAME).cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean			:
	$(RM) $(NAME).o $(NAME)


install		: $(NAME)
	echo "not implemented"
