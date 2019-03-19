# Makefile for Basler Pylon sample program
.PHONY			: all clean install

# The program to build
NAME			:= checkcode
OUTPUT   	:= $(NAME)


# Installation directories for GenICam and Pylon
PYLON_ROOT		?= /opt/pylon5
GENICAM_ROOT	?= $(PYLON_ROOT)/genicam

# Build tools and flags
CXX				?= g++

LD         := $(CXX)
CPPFLAGS   :=   $(shell pkg-config zbar --cflags) $(shell pkg-config opencv --cflags) $(shell pkg-config tesseract --cflags)
CXXFLAGS   := -std=c++14 -O3 -Ofast #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    :=
LDLIBS     :=  $(shell pkg-config zbar --cflags --libs) $(shell pkg-config opencv --libs) $(shell pkg-config tesseract --libs)

LDLIBS     += -lboost_system
LDLIBS     += -lboost_regex
LDLIBS     += -lboost_filesystem
LDLIBS     += -lboost_chrono
LDLIBS     += -lboost_iostreams
LDLIBS     += -lboost_date_time
LDLIBS     += -lpthread



UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
LDLIBS     += -lboost_thread
LDLIBS     += -lboost_atomic
LDFLAGS    += $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS    += $(shell $(PYLON_ROOT)/bin/pylon-config --libs)
CPPFLAGS    += $(shell $(PYLON_ROOT)/bin/pylon-config --cflags)
endif
ifeq ($(UNAME), Darwin)
LDLIBS     += -lboost_thread-mt
LDLIBS     += -lboost_atomic-mt
CPPFLAGS += -I/Library/Frameworks/pylon.framework/Headers/
CPPFLAGS += -I/Library/Frameworks/pylon.framework/Headers/GenICam/
LDFLAGS  +=-Wl,-rpath,/Library/Frameworks
LDLIBS   += -F"/Library/Frameworks/" -framework pylon
endif

# Rules for building
all				: $(NAME)

$(NAME)			: $(NAME).o
	$(LD) $(LDFLAGS) -v -o $@ checkcode.o $(LDLIBS)

SOURCES := $(shell find . -name '*.cpp')
HEADERS := $(shell find . -name '*.h')



$(NAME).o: $(SOURCES) $(HEADERS)

	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./checkcode.cpp


	#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES)

clean			:
	$(RM) *.o $(NAME)


