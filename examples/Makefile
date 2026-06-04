CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -g
LDFLAGS ?=

# 头文件与库路径可通过环境变量覆盖
SFS_INCLUDE ?= ../include
SFS_LIBDIR  ?=
SFS_LIBS    ?= -lsfs_client

INCLUDES = -I$(SFS_INCLUDE)
LIBPATH  = $(if $(SFS_LIBDIR),-L$(SFS_LIBDIR),)

EXAMPLES   = file_rw_example list_dir_example getattr_example
COMMON_SRC = ns_common.c

.PHONY: all clean

all: $(EXAMPLES)

$(EXAMPLES): %: %.c $(COMMON_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(COMMON_SRC) $(LIBPATH) $(LDFLAGS) $(SFS_LIBS)

clean:
	rm -f $(EXAMPLES)
