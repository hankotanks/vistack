CC := gcc
AR := ar

EXAMPLES := cblas_example1 cblas_example2

ROOT := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

DIR_INC := $(ROOT)/include
DIR_SRC := $(ROOT)/src
DIR_OBJ := $(ROOT)/build
DIR_BIN := $(ROOT)/bin
ifeq ($(OS), Windows_NT)
	DIR_LIB := $(DIR_BIN)/photogram.lib
else
	DIR_LIB := $(DIR_BIN)/libphotogram.a
endif

DIR_CBLAS := $(ROOT)/OpenBLAS/lapack-netlib/CBLAS
DIR_CBLAS_INC := $(DIR_CBLAS)/include
DIR_CBLAS_SRC := $(DIR_CBLAS)/src
ifeq ($(OS), Windows_NT)
	DIR_CBLAS_LIB := $(DIR_BIN)/cblas.lib
else
	DIR_CBLAS_LIB := $(DIR_BIN)/libcblas.a
endif

CFLAGS := -Wall -Wextra -Wconversion -Wpedantic 
CFLAGS += -L$(DIR_BIN) -I$(DIR_INC) -isystem$(DIR_CBLAS_INC) -DLOGGING

all: lib examples

examples: cblas lib $(EXAMPLES)

$(EXAMPLES):
	$(CC) $(CFLAGS) $(ROOT)/examples/$@.c -o $(DIR_BIN)/$@ -lphotogram -lcblas -lblas

lib: cblas $(patsubst $(DIR_SRC)/%.c, $(DIR_OBJ)/%.o, $(wildcard $(DIR_SRC)/*.c))
	$(AR) rcs $(DIR_LIB) $(filter-out $(firstword $^), $^)

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

cblas:
	@-$(MAKE) -s -k -C $(DIR_CBLAS_SRC) CBLASLIB=$(DIR_CBLAS_LIB)

clean:
	$(RM) -r $(DIR_OBJ)/*.o
	@-$(MAKE) cleanobj -s -k -C $(DIR_CBLAS_SRC)

.PHONY: all examples lib cblas clean