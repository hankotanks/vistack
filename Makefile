LIB := vistack

CC := gcc
AR := ar

EXAMPLES := flame_example

CFLAGS := -Wall -Wextra -Wconversion -Wpedantic -DLOGGING

#
#
#

DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
DIR_INC := $(DIR)/include
DIR_SRC := $(DIR)/src
DIR_OBJ := $(DIR)/build
DIR_BIN := $(DIR)/bin
DIR_EXT := $(DIR)/ext
DIR_LIB := $(DIR)/lib
ifeq ($(OS), Windows_NT)
	DIR_LIB_FULL := $(DIR_LIB)/$(LIB).lib
else
	DIR_LIB_FULL := $(DIR_LIB)/lib$(LIB).a
endif

CFLAGS += -I$(DIR_INC) -isystem$(DIR_EXT)
LDLIBS := -l$(LIB) -lflame -lm

LDFLAGS := -L$(DIR)/lib

EXT_FLAME ?= 0
ifeq ($(EXT_FLAME), 0)
	DIR_FLAME := $(DIR)/libflame
endif

#
#
#

all: lib examples

examples: flame lib $(EXAMPLES)

$(EXAMPLES):
	$(CC) $(CFLAGS) $(DIR)/examples/$@.c -o $(DIR_BIN)/$@ -l$(LIB) $(LDFLAGS) $(LDLIBS)

lib: flame $(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
	$(AR) rcs $(DIR_LIB_FULL) $(filter-out $(firstword $^), $^)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LDLIBS)

ifeq ($(EXT_FLAME), 0)
flame:
	$(MAKE) -C $(DIR_FLAME)
	$(MAKE) install -C $(DIR_FLAME)
init:
	cd $(DIR_FLAME) && ./configure --enable-builtin-blas --enable-cblas-interfaces --disable-non-critical-code --libdir=$(DIR_LIB) --includedir=$(DIR_EXT)
else
flame:
init:
endif

ifeq ($(EXT_FLAME), 0)
clean:
	$(RM) -r $(DIR_OBJ)/*.o
	$(MAKE) clean -C $(DIR_FLAME)
else
clean:
	$(RM) -r $(DIR_OBJ)/*.o
endif

.PHONY: all examples lib flame init clean