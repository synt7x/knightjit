EXECUTABLE := knight

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
.SHELLFLAGS := /c
endif

ARCH := X64
CFLAGS := -O3 -march=native -mtune=native -Wall -Wextra -std=c99
CFLAGS += -finline-functions -fno-stack-protector
CFLAGS += -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -DJIT_OFF
CC := clang

GIT := git
LUA := luajit

ifeq (, $(shell where $(CC)))
$(error "C compiler '$(CC)' not found in PATH")
endif

ifeq (, $(shell where $(GIT)))
$(error "Git '$(GIT)' not found in PATH")
endif

ifeq ($(OS),Windows_NT)
	RM := rd /s /q
	EXISTS := if not exist
	MKDIR := mkdir
	EXECUTABLE := $(EXECUTABLE).exe
	LDFLAGS := -Wl,/SUBSYSTEM:CONSOLE -Wl,/OPT:REF -Wl,/OPT:ICF -g
	CFLAGS += -D_CRT_SECURE_NO_WARNINGS -g
else
	RM := rm -rf
	EXISTS := test -d
	MKDIR := || mkdir -p
	GIT := || $(GIT)
	LDFLAGS := -Wl,--gc-sections -s -Wl,--strip-all
endif

TARGET := target
ARTIFACTS := $(TARGET)/artifacts
LUAJIT := $(TARGET)/luajit
DYNASM := $(LUAJIT)/dynasm
SRC := src
JIT := $(SRC)/jit

ifeq (, $(shell where $(LUA)))
LUA := $(LUAJIT)/minilua-$(EXECUTABLE)
NEED_LUA := 1
endif

SOURCES := $(wildcard $(SRC)/*.c)
JIT_SOURCES := $(wildcard $(JIT)/*.c)
INCLUDE := $(wildcard $(SRC)/*.h)
PRE := $(patsubst $(JIT)/%.c,$(ARTIFACTS)/%.$(ARCH).c,$(JIT_SOURCES))
OBJECTS := $(patsubst $(SRC)/%.c,$(ARTIFACTS)/%.o,$(SOURCES)) $(patsubst $(ARTIFACTS)/%.$(ARCH).c,$(ARTIFACTS)/%.o,$(PRE))

.PHONY: all clean deps

all: build

build: deps rebuild $(OBJECTS)
	@echo ==== Building $(EXECUTABLE) ====
	$(CC) -o $(TARGET)/$(EXECUTABLE) $(OBJECTS) $(LDFLAGS)
	@echo ==== Built $(TARGET)/$(EXECUTABLE) ====

$(ARTIFACTS)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $(SRC)/$*.c -o $@ -I$(SRC) -I$(JIT)

$(ARTIFACTS)/%.o: $(ARTIFACTS)/%.$(ARCH).c
	$(CC) $(CFLAGS) -c $(ARTIFACTS)/$*.$(ARCH).c -o $@

$(ARTIFACTS)/%.$(ARCH).c: $(JIT)/%.c
	$(LUA) $(DYNASM)/dynasm.lua -D $(ARCH) -o $@ $<

deps:
	@echo ==== Installing dependencies ====
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"
	$(EXISTS) $(TARGET) $(MKDIR) $(TARGET)
	$(EXISTS) $(LUAJIT) $(GIT) clone https://github.com/LuaJIT/LuaJIT.git $(LUAJIT)

ifeq ($(NEED_LUA),1)
	@echo ==== Building minilua ====
	$(CC) -o $(LUA) $(CFLAGS) $(LUAJIT)/src/host/minilua.c
endif

rebuild:
	$(RM) "$(ARTIFACTS)"
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"

clean:
	$(RM) target