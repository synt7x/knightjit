EXECUTABLE := knight

ARCH := X64
CFLAGS := -O3 -Wall -Wextra -std=c99 -D_CRT_SECURE_NO_WARNINGS
CC := clang

GIT := git
LUA := luajitb

ifeq (, $(shell where $(CC)))
$(error "C compiler '$(CC)' not found in PATH")
endif

ifeq (, $(shell where $(GIT)))
$(error "Git '$(GIT)' not found in PATH")
endif

ifeq ($(OS),Windows_NT)
	RM := rd /s /q
	EXISTS := if not exist
	MKDIR = mkdir
	EXECUTABLE := $(EXECUTABLE).exe
else
	RM := rm -rf
	EXISTS := test -d
	MKDIR := || mkdir -p
	GIT := || $(GIT)
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
	$(CC) -o $(TARGET)/$(EXECUTABLE) $(OBJECTS)
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