# Makefile for Basler Pylon sample program
.PHONY			: all clean install

# The program to build
NAME			:= grabzbar
OUTPUT   	:= $(NAME)


# Installation directories for GenICam and Pylon
PYLON_ROOT		?= /opt/pylon5
GENICAM_ROOT	?= $(PYLON_ROOT)/genicam

# Build tools and flags
CXX				?= g++

LD         := $(CXX)
CPPFLAGS   := $(shell mysql_config --cflags) $(shell pkg-config zbar --cflags) $(shell pkg-config opencv --cflags) $(shell pkg-config tesseract --cflags)
CXXFLAGS   := -std=c++11 #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    :=
LDLIBS     := $(shell mysql_config --libs) $(shell pkg-config zbar --cflags --libs) $(shell pkg-config opencv --libs) $(shell pkg-config tesseract --libs)

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
	$(LD) $(LDFLAGS) -v -o $@ RegionOfInterest.o ExtractAddress.o glob_posix.o ImageRecognizeEx.o ocr_ext.o server.o request_parser.o request_handler.o reply.o mjpeg_server.o mime_types.o connection.o Grabber.o grabzbar.o $(LDLIBS)

SOURCES := $(shell find . -name '*.cpp')
HEADERS := $(shell find . -name '*.h')



$(NAME).o: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/server.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/request_parser.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/request_handler.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/reply.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/mjpeg_server.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/mime_types.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./mjpeg/connection.cpp

	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./ocrs/new/RegionOfInterest.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./ocrs/new/glob_posix.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./ocrs/new/ExtractAddress.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./ocrs/new/ImageRecognizeEx.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./ocrs/new/ocr_ext.cpp


	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./src/Grabber.cpp

	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./src/grabzbar.cpp


	#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES)

clean			:
	$(RM) *.o $(NAME)


install		: $(NAME)
	mkdir /opt/grab
	cp $(NAME) /opt/grab/$(NAME)
	echo "copy file $(NAME) to /usr/bin/$(NAME)"
