JIT_ENABLED ?= JIT_OFF
STANDARD ?= STANDARD

EXECUTABLE ?= knight

ARCH ?= X64
COMPILER ?= clang

SEP := /

CFLAGS ?= -O3 -march=native -mtune=native
CFLAGS += -Wall -Wextra -std=c99
CFLAGS += -finline-functions -fno-stack-protector
CFLAGS += -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -D$(JIT_ENABLED) -D$(STANDARD) -D$(ARCH)

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
TESTS := tests
JIT := $(SRC)/jit

ifeq (, $(shell $(WHICH) $(LUA)))
LUA := $(LUAJIT)/minilua-$(EXECUTABLE)
NEED_LUA := 1
endif

SOURCES := $(wildcard $(SRC)/*.c)
JIT_SOURCES := $(wildcard $(JIT)/*.c)
PRE := $(patsubst $(JIT)/%.c,$(ARTIFACTS)/%.$(ARCH).c,$(JIT_SOURCES))
OBJECTS := $(patsubst $(SRC)/%.c,$(ARTIFACTS)/%.o,$(SOURCES)) $(patsubst $(ARTIFACTS)/%.$(ARCH).c,$(ARTIFACTS)/%.o,$(PRE))

.PHONY: all clean deps build rebuild test strict increment

all: build

build: deps rebuild $(OBJECTS)
	@echo ==== Building $(EXECUTABLE) ====
	$(COMPILER) -o $(TARGET)/$(EXECUTABLE) $(OBJECTS) $(LDFLAGS)
	@echo ==== Built $(TARGET)/$(EXECUTABLE) ====

$(ARTIFACTS)/%.o: $(SRC)/%.c
	$(COMPILER) $(CFLAGS) -c $(SRC)/$*.c -o $@ -I$(SRC) -I$(JIT) -I$(DYNASM)

$(ARTIFACTS)/reg.o: $(JIT)/reg.c
	$(COMPILER) $(CFLAGS) -c $< -o $@ -I$(SRC) -I$(JIT) -I$(DYNASM)

$(ARTIFACTS)/%.o: $(ARTIFACTS)/%.$(ARCH).c
	$(COMPILER) $(CFLAGS) -c $(ARTIFACTS)/$*.$(ARCH).c -o $@ -I$(SRC) -I$(JIT) -I$(DYNASM)

# JIT Targets
$(ARTIFACTS)/x86_64.$(ARCH).c: $(JIT)/x86_64.c
	$(LUA) $(DYNASM)/dynasm.lua -D $(ARCH) -o $@ $<

$(ARTIFACTS)/x86.$(ARCH).c: $(JIT)/x86.c
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

rebuild:
	$(RM) "$(ARTIFACTS)"
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"

clean:
	$(RM) target

test:
	$(LUA) $(TESTS)/test.lua $(TARGET)$(SEP)$(EXECUTABLE) -stop

strict:
	$(LUA) $(TESTS)/test.lua $(TARGET)$(SEP)$(EXECUTABLE)

increment: build test