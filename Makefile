LIB := vistack

CC := gcc
AR := ar

EXAMPLES := test corners

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
LDLIBS := -l$(LIB) -lflame

BUILD_FLAME ?= 0
ifneq ($(BUILD_FLAME), 0)
	DIR_LIB_FLA  := $(DIR)/libflame
	DIR_LIB_BLIS := $(DIR)/blis
endif

BUILD_BLIS ?= $(BUILD_FLAME)
ifeq ($(BUILD_BLIS), 0)
	LDLIBS += -lblas
else
	LDLIBS += -lblis
endif
LDLIBS += -lm

LDFLAGS := -L$(DIR)/lib

ifeq ($(BUILD_FLAME), 0)
ifneq ($(BUILD_BLIS), 0)
$(error Building BLIS when using system's libflame package is unsupported.)
endif
endif

#
#
#

.PHONY: init
init: dir dep all

.PHONY: all
all: lib examples

.PHONY: examples
examples: dep lib $(EXAMPLES)

$(EXAMPLES):
	$(CC) $(CFLAGS) $(DIR)/examples/$@.c -o $(DIR_BIN)/$@ \
	-l$(LIB) $(LDFLAGS) $(LDLIBS) $(shell $(MAKE) get_bin_flags -s -C glenv)

.PHONY: lib
lib: dep \
$(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
	$(AR) rcs $(DIR_LIB_FULL) $(filter-out $(firstword $^), $^)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(shell $(MAKE) get_obj_flags -s -C glenv) $(LDFLAGS) $(LDLIBS)

.PHONY: dir
dir:
	mkdir -p $(DIR_BIN) $(DIR_OBJ) $(DIR_LIB) $(DIR_LOG)

.PHONY: clean
clean:
	$(RM) -r $(DIR_OBJ)/*.o

.PHONY: dep
ifneq ($(BUILD_FLAME), 0)
ifneq ($(BUILD_BLIS), 0)
dep: dep-fla dep-blis
else
dep: dep-fla
endif
else
dep:
endif

ifneq ($(BUILD_BLIS), 0)
.PHONY: dep-blis
dep-blis:
	cd $(DIR_LIB_BLIS) && ./configure --disable-shared --enable-arg-max-hack \
	--sharedir=$(DIR_OBJ) --libdir=$(DIR_LIB) --includedir=$(DIR_EXT) auto
	$(MAKE) -j$(BUILD_FLAME) -C $(DIR_LIB_BLIS)
	$(MAKE) install -C $(DIR_LIB_BLIS)
	$(MAKE) clean -C $(DIR_LIB_BLIS)
	$(RM) $(DIR_LIB_BLIS)/blis.pc
endif

ifneq ($(BUILD_FLAME), 0)
.PHONY: dep-fla
dep-fla:
	cd $(DIR_LIB_FLA) && ./configure --disable-non-critical-code \
	--libdir=$(DIR_LIB) --includedir=$(DIR_EXT)
	$(MAKE) -j$(BUILD_FLAME) -C $(DIR_LIB_FLA)
	$(MAKE) install -C $(DIR_LIB_FLA)
	$(MAKE) clean -C $(DIR_LIB_FLA)
	$(RM) $(DIR_LIB_FLA)/ar_obj_list
endif
