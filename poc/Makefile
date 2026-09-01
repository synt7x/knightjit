EXECUTABLE ?= poc

ARCH ?= X64
COMPILER ?= clang++

CFLAGS ?= -O3 -march=native -mtune=native
CFLAGS += -Wall -Wextra -std=c++26
CFLAGS += -finline-functions -fno-stack-protector
CFLAGS += -ffunction-sections -fdata-sections -fno-builtin

GIT ?= git
LUA ?= luajit

ifeq ($(OS),Windows_NT)
	RM := rd /s /q
	WHICH := where
	EXISTS := if not exist
	MKDIR := mkdir
	EXECUTABLE := $(EXECUTABLE).exe
 	LDFLAGS := -Wl,/SUBSYSTEM:CONSOLE -Wl,/OPT:REF -Wl,/OPT:ICF -g
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS -g

	SEP := \\
	SHELL := cmd.exe
	.SHELLFLAGS := /c
else
	RM := rm -rf
	WHICH := which
	EXISTS := test -d
	MKDIR := || mkdir -p
	GIT := || $(GIT)
	LDFLAGS := -Wl,--gc-sections -s -Wl,--strip-all
endif

ifeq (, $(shell $(WHICH) $(COMPILER)))
COMPILER ?= $(CC)
endif

ifeq (, $(shell $(WHICH) $(COMPILER)))
$(error "C compiler '$(COMPILER)' not found in PATH")
endif

ifeq (, $(shell $(WHICH) $(GIT)))
$(error "Git '$(GIT)' not found in PATH")
endif

TARGET := target
ARTIFACTS := $(TARGET)/artifacts
LUAJIT := $(TARGET)/luajit
DYNASM := $(LUAJIT)/dynasm
SRC := src

SOURCES := $(SRC)/main.cpp $(SRC)/trace.cpp
JIT_SOURCES := $(SRC)/stage0.cpp
OBJECTS := $(ARTIFACTS)/main.o $(ARTIFACTS)/trace.o $(ARTIFACTS)/stage0.$(ARCH).o

.PHONY: all clean deps build

all: build

build: deps $(OBJECTS)
	@echo ==== Building $(EXECUTABLE) ====
	$(COMPILER) -o $(TARGET)/$(EXECUTABLE) $(OBJECTS) $(LDFLAGS)
	@echo ==== Built $(TARGET)/$(EXECUTABLE) ====

$(ARTIFACTS)/%.o: $(SRC)/%.cpp
	$(COMPILER) $(CFLAGS) -c $(SRC)/$*.cpp -o $@ -I$(DYNASM) -I$(SRC)

$(ARTIFACTS)/%.$(ARCH).o: $(ARTIFACTS)/%.$(ARCH).cpp
	$(COMPILER) $(CFLAGS) -c $(ARTIFACTS)/$*.$(ARCH).cpp -o $@ -I$(DYNASM) -I$(SRC)

$(ARTIFACTS)/stage0.$(ARCH).cpp: $(SRC)/stage0.cpp
	$(LUA) $(DYNASM)/dynasm.lua -D $(ARCH) -o $@ $<

deps:
	@echo ==== Installing dependencies ====
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"
	$(EXISTS) $(TARGET) $(MKDIR) $(TARGET)
	$(EXISTS) $(LUAJIT) $(GIT) clone https://github.com/LuaJIT/LuaJIT.git $(LUAJIT)

ifeq ($(NEED_LUA),1)
	@echo ==== Building minilua ====
	$(COMPILER) -o $(LUA) $(CFLAGS) $(LUAJIT)/src/host/minilua.c -lm
endif

clean:
	$(RM) target