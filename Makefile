VERSION := 2.0.0
EXECUTABLE ?= knight

ARCH ?= x86_64
CC := clang++

CFLAGS ?= -O3 -march=native -mtune=native \
		  -Wall -Wextra -flto -DNDEBUG \
		  -fno-stack-protector -ffunction-sections -fdata-sections \
		  -funroll-loops -fstrict-aliasing -std=c++26 -ffast-math
LDFLAGS ?= -flto -fuse-ld=lld

GIT ?= git
JANET ?= janet

TARGET ?= target
ARTIFACTS ?= $(TARGET)/artifacts
SRC ?= src

ifeq ($(OS), Windows_NT)
	EXECUTABLE := $(EXECUTABLE).exe
	LDFLAGS += -Wl,/SUBSYSTEM:CONSOLE -Wl,/OPT:REF -Wl,/OPT:ICF
endif

SOURCES := $(wildcard $(SRC)/*.cpp)
SOURCES += $(wildcard $(SRC)/jit/*.cpp)
SOURCES += $(wildcard $(SRC)/logs/*.cpp)
SOURCES += $(wildcard $(SRC)/parser/*.cpp)
SOURCES += $(wildcard $(SRC)/vm/*.cpp)

INCLUDES := -I$(SRC)
INCLUDES += -I$(SRC)/jit
INCLUDES += -I$(SRC)/logs
INCLUDES += -I$(SRC)/parser
INCLUDES += -I$(SRC)/vm

OBJECTS := $(patsubst %.cpp, $(ARTIFACTS)/%.o, $(notdir $(SOURCES)))

RM := rm -rf
EXISTS := test -d
MKDIR := || mkdir -p

ifeq ($(OS), Windows_NT)
RM := rd /s /q
EXISTS := if not exist
MKDIR := mkdir
endif

ifeq ($(OS), Windows_NT)
SHELL := cmd.exe
.SHELLFLAGS := /c
endif

GIT_HASH := $(shell git rev-parse --short HEAD)

DEFINES := -DKNIGHT_VERSION=\"$(VERSION)\"
DEFINES += -DKNIGHT_GIT_HASH=\"$(GIT_HASH)\"
CFLAGS += $(DEFINES)

all: build
build: $(TARGET)/$(EXECUTABLE)

$(TARGET)/$(EXECUTABLE): $(OBJECTS) | $(TARGET)
	$(CC) $(CFLAGS) $^ -o $(TARGET)/$(EXECUTABLE) $(LDFLAGS)

$(ARTIFACTS)/%.o: $(SRC)/%.cpp | $(ARTIFACTS)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

$(ARTIFACTS)/%.o: $(SRC)/jit/%.cpp | $(ARTIFACTS)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

$(ARTIFACTS)/%.o: $(SRC)/logs/%.cpp | $(ARTIFACTS)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

$(ARTIFACTS)/%.o: $(SRC)/parser/%.cpp | $(ARTIFACTS)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

$(ARTIFACTS)/%.o: $(SRC)/vm/%.cpp | $(ARTIFACTS)
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

$(TARGET):
	$(EXISTS) "$(TARGET)" $(MKDIR) "$(TARGETS)"

$(ARTIFACTS):
	$(EXISTS) "$(ARTIFACTS)" $(MKDIR) "$(ARTIFACTS)"

clean:
	$(RM) target

.PHONY: all build clean