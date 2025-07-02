LIB := vistack

CC := gcc
AR := ar

EXAMPLES := flame_example image_test

CFLAGS := -ggdb3 -Wall -Wextra -Wconversion -Wpedantic -DLOGGING

#
#
#

ifeq ($(OS), Windows_NT)
	$(error Windows is unsupported. Exiting.)
endif

DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
DIR_INC := $(DIR)/include
DIR_SRC := $(DIR)/src
DIR_OBJ := $(DIR)/build
DIR_BIN := $(DIR)/bin
DIR_EXT := $(DIR)/ext
DIR_LIB := $(DIR)/lib
DIR_LIB_FULL := $(DIR_LIB)/lib$(LIB).a
DIR_LOG := $(DIR)/logs

CFLAGS += -I$(DIR_INC) -isystem$(DIR_EXT)
LDLIBS := -l$(LIB) -lflame -lblas -lm

LDFLAGS := -L$(DIR)/lib

WITH_INTERNAL_FLA ?= 0
ifneq ($(WITH_INTERNAL_FLA), 0)
	DIR_FLA := $(DIR)/libflame
endif

#
#
#

.PHONY: init
init: dir flame-init all

.PHONY: all
all: lib examples

.PHONY: examples
examples: flame lib $(EXAMPLES)

$(EXAMPLES):
	$(CC) $(CFLAGS) $(DIR)/examples/$@.c -o $(DIR_BIN)/$@ \
	-l$(LIB) $(LDFLAGS) $(LDLIBS) $(shell $(MAKE) get_bin_flags -s -C glenv)

.PHONY: lib
lib: flame \
$(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
	$(AR) rcs $(DIR_LIB_FULL) $(filter-out $(firstword $^), $^)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(shell $(MAKE) get_obj_flags -s -C glenv) $(LDFLAGS) $(LDLIBS)

.PHONY: dir
dir:
	mkdir -p $(DIR_BIN) $(DIR_OBJ) $(DIR_LIB)

.PHONY: clean
clean: flame-clean
	$(RM) -r $(DIR_OBJ)/*.o

.PHONY: flame flame-init flame-clean
ifneq ($(WITH_INTERNAL_FLA), 0)
flame:
	$(MAKE) -C $(DIR_FLA)
	$(MAKE) install -C $(DIR_FLA)
	$(RM) $(DIR_FLA)/ar_obj_list
flame-init:
	cd $(DIR_FLA) && ./configure --disable-non-critical-code \
	--libdir=$(DIR_LIB) --includedir=$(DIR_EXT)
flame-clean:
	$(MAKE) clean -C $(DIR_FLA)
else
flame:
flame-init:
flame-clean:
endif