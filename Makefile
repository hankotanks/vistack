LIB := vistack

CC := gcc
AR := ar

EXAMPLES := flame_example

CFLAGS := -Wall -Wextra -Wconversion -Wpedantic -DLOGGING

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

CFLAGS += -I$(DIR_INC) -isystem$(DIR_EXT)
LDLIBS := -l$(LIB) -lflame -lblas -lm

LDFLAGS := -L$(DIR)/lib

EXT_FLAME ?= 0
ifeq ($(EXT_FLAME), 0)
	DIR_FLAME := $(DIR)/libflame
endif

#
#
#

.PHONY: all
all: lib examples

.PHONY: examples
examples: flame lib $(EXAMPLES)

$(EXAMPLES):
	$(CC) $(CFLAGS) $(DIR)/examples/$@.c -o $(DIR_BIN)/$@ \
	-l$(LIB) $(LDFLAGS) $(LDLIBS)

.PHONY: lib
lib: flame \
$(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
	$(AR) rcs $(DIR_LIB_FULL) $(filter-out $(firstword $^), $^)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LDLIBS)

.PHONY: init
init: dir flame-init lib examples

.PHONY: dir
dir:
	mkdir -p $(DIR_BIN) $(DIR_OBJ) $(DIR_LIB)

.PHONY: clean
clean: flame-clean
	$(RM) -r $(DIR_OBJ)/*.o

.PHONY: flame flame-init flame-clean
ifeq ($(EXT_FLAME), 0)
flame:
	$(MAKE) -C $(DIR_FLAME)
	$(MAKE) install -C $(DIR_FLAME)
	$(RM) $(DIR_FLAME)/ar_obj_list
flame-init:
	cd $(DIR_FLAME) && ./configure --disable-non-critical-code \
	--libdir=$(DIR_LIB) --includedir=$(DIR_EXT)
flame-clean:
	$(MAKE) clean -C $(DIR_FLAME)
else
flame:
flame-init:
flame-clean:
endif