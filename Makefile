LIB := vistack

CC := gcc
AR := ar

EXAMPLES := cblas_example1

CFLAGS := -Wall -Wextra -Wconversion -Wpedantic -DLOGGING

#
#
#

DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
DIR_INC := $(DIR)/include
DIR_SRC := $(DIR)/src
DIR_OBJ := $(DIR)/build
DIR_BIN := $(DIR)/bin
ifeq ($(OS), Windows_NT)
	DIR_LIB := $(DIR_BIN)/$(LIB).lib
else
	DIR_LIB := $(DIR_BIN)/lib$(LIB).a
endif

CFLAGS += -I$(DIR_INC)
LDLIBS := -l$(LIB)

LDFLAGS := -L$(DIR_BIN)

EXT_CBLAS ?= 0
ifeq ($(EXT_CBLAS), 0)
	DIR_CBLAS := $(DIR)/OpenBLAS/lapack-netlib/CBLAS
	DIR_CBLAS_INC := $(DIR_CBLAS)/include
	DIR_CBLAS_SRC := $(DIR_CBLAS)/src
	ifeq ($(OS), Windows_NT)
		DIR_CBLAS_LIB := $(DIR_BIN)/cblas.lib
	else
		DIR_CBLAS_LIB := $(DIR_BIN)/libcblas.a
	endif
	CFLAGS += -isystem$(DIR_CBLAS_INC)
	LDLIBS += -lcblas -lblas
else
	LDLIBS += -lopenblas
endif

#
#
#

all: lib examples

examples: cblas lib $(EXAMPLES)

$(EXAMPLES):
	$(CC) $(CFLAGS) $(DIR)/examples/$@.c -o $(DIR_BIN)/$@ -l$(LIB) $(LDFLAGS) $(LDLIBS)

lib: cblas $(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
	$(AR) rcs $(DIR_LIB) $(filter-out $(firstword $^), $^)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LDLIBS)

ifeq ($(EXT_CBLAS), 0)
cblas:
	@-$(MAKE) -s -k -C $(DIR_CBLAS_SRC) CBLASLIB=$(DIR_CBLAS_LIB)
else
cblas:
endif

ifeq ($(EXT_CBLAS), 0)
clean:
	$(RM) -r $(DIR_OBJ)/*.o
	@-$(MAKE) cleanobj -s -k -C $(DIR_CBLAS_SRC)
else
clean:
	$(RM) -r $(DIR_OBJ)/*.o
endif

.PHONY: all examples lib cblas clean