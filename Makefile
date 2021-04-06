CXX      = g++
CC       = gcc
OBJ      = main.o
LINKOBJ  = main.o
CFLAGS = -std=c++17 -Wall -g -c -Wno-parentheses
LIBS = `pkg-config gtkmm-3.0 dbus-cxx-1.0 --cflags --libs` -lboost_system
BIN	 = konekoe-session-listener

mkfile_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

.PHONY: all all-before all-after clean clean-custom default
default: build ;
all: build ;

install: $(BIN)
	mkdir -p $(DESTDIR)usr/bin && cp $(BIN) $(DESTDIR)usr/bin/$(BIN)
	mkdir -p $(DESTDIR)usr/lib/systemd/user && cp $(BIN).service $(DESTDIR)usr/lib/systemd/user/$(BIN).service

build: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(LINKOBJ) -o $(BIN) $(LIBS)

./%.o: ./%.cpp
	$(CXX) $(CFLAGS) $< -o $@ $(LIBS)

.PHONY : clean
clean :
	-rm -rf $(OBJ) $(BIN) usr/
