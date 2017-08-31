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
CPPFLAGS   := $(shell mysql_config --cflags) $(shell $(PYLON_ROOT)/bin/pylon-config --cflags) $(shell pkg-config zbar --cflags) $(shell pkg-config opencv --cflags) $(shell pkg-config tesseract --cflags)
CXXFLAGS   := -std=c++11 #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell mysql_config --libs) $(shell $(PYLON_ROOT)/bin/pylon-config --libs) $(shell pkg-config zbar --cflags --libs) $(shell pkg-config opencv --libs) $(shell pkg-config tesseract --libs) -lboost_system -lboost_regex -lboost_filesystem -lboost_thread -lboost_chrono -lboost_iostreams -lboost_atomic -lboost_date_time -lpthread


# /usr/local/lib/libboost_thread-mt.dylib;
# /usr/local/lib/libboost_system-mt.dylib;
# /usr/local/lib/libboost_regex-mt.dylib;
# /usr/local/lib/libboost_chrono-mt.dylib;
# /usr/local/lib/libboost_date_time-mt.dylib;
# /usr/local/lib/libboost_atomic-mt.dylib
# Rules for building
all				: $(NAME)

$(NAME)			: $(NAME).o
	$(LD) $(LDFLAGS) -o $@ RegionOfInterest.o ExtractAddress.o glob_posix.o ImageRecognizeEx.o server.o request_parser.o request_handler.o reply.o mjpeg_server.o mime_types.o connection.o grabzbar.o $(LDLIBS)

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
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./ocrs/new/ImageRecognizeEx.cpp

	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c ./src/grabzbar.cpp


	#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES)

clean			:
	$(RM) *.o $(NAME)


install		: $(NAME)
	echo "not implemented"
