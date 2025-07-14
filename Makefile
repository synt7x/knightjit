EXECUTABLE := knight

ARCH := X64
CC := clang
CFLAGS := -O3 -Wall -Wextra -std=c99

GIT := git
LUA := luajit

ifeq ($(OS),Windows_NT)
	RM := rd /s /q
	EXISTS := if not exist
	MKDIR = mkdir
	EXECUTABLE := $(EXECUTABLE).exe
else
	RM := rm -rf
	EXISTS := test -d
	MKDIR := || mkdir -p
endif

TARGET := target
ARTIFACTS := $(TARGET)/artifacts
LUAJIT := $(TARGET)/luajit
DYNASM := $(LUAJIT)/dynasm
SRC := src
JIT := $(SRC)/jit

SOURCES := $(wildcard $(SRC)/*.c)
JIT_SOURCES := $(wildcard $(JIT)/*.c)
INCLUDE := $(wildcard $(SRC)/*.h)
PRE := $(patsubst $(JIT)/%.c,$(ARTIFACTS)/%.$(ARCH).c,$(JIT_SOURCES))
OBJECTS := $(patsubst $(SRC)/%.c,$(ARTIFACTS)/%.o,$(SOURCES)) $(patsubst $(ARTIFACTS)/%.$(ARCH).c,$(ARTIFACTS)/%.o,$(PRE))

.PHONY: all clean deps

all: build

build: deps rebuild $(OBJECTS)
	$(CC) -o $(TARGET)/$(EXECUTABLE) $(OBJECTS)

# Rule for object files from normal sources
$(ARTIFACTS)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $(SRC)/$*.c -o $@ -I$(SRC) -I$(JIT)

# Rule for object files from JIT sources (require generated .c file)
$(ARTIFACTS)/%.o: $(ARTIFACTS)/%.$(ARCH).c
	$(CC) $(CFLAGS) -c $(ARTIFACTS)/$*.$(ARCH).c -o $@

# Rule to generate JIT source .c files
$(ARTIFACTS)/%.$(ARCH).c: $(JIT)/%.c
	$(LUA) $(DYNASM)/dynasm.lua -D $(ARCH) -o $@ $<

deps:
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"
	$(EXISTS) $(TARGET) $(MKDIR) $(TARGET)
	$(EXISTS) $(LUAJIT) git clone https://github.com/LuaJIT/LuaJIT.git $(LUAJIT)

rebuild:
	$(RM) "$(ARTIFACTS)"
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"

clean:
	$(RM) target