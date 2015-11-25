CC=gcc# GNU C Compiler
THISFILEDIR=$(dir $(lastword $(MAKEFILE_LIST)))# get the directory path of this makefile from current execution directory path
BASEDIR=$(THISFILEDIR)../# very important.. this will be the heart of every operation. step up to the project root dir.
SRCDIR=$(BASEDIR)src/# Source files directory
OBJDIR=$(BASEDIR)obj/# Object files directory
INCDIR=$(BASEDIR)include/# Header files directory
BUILDDIR=$(BASEDIR)build/# Output directory
INSTALLDIR=/usr/local/lib
INSTALLHDIR=/usr/local/include
OBJDIRS=$(patsubst $(SRCDIR)%,$(OBJDIR)%,${shell find $(SRCDIR) -type d -print})# Directory tree of for object files, based on source files
REQDIRS+=$(OBJDIRS)# Add object directory tree to required directory
REQDIRS+=$(BUILDDIR)# Add build directory tree to required directory
LIBNAME=libcornet.so# output library file name
IFLAGS=-I$(INCDIR)# Object compilation flags to add search directory for headers
OFLAGS=-o# Flag for defining output file name.
LDFLAGS=-Wall -shared# Library compilation flags to indicate that we make shared library
CFLAGS=-fPIC -Wall $(IFLAGS) -c# Object compilation flags by order, Position Independent Code, Warning all, header dir, make object
DEPS=$(wildcard ${shell find $(INCDIR) -iname "*.h" -print})# Header files
SRCS=$(wildcard ${shell find $(SRCDIR) -iname "*.c" -print})# C Source files
OBJS=$(patsubst $(SRCDIR)%.c,$(OBJDIR)%.o,$(SRCS))# C Object files

.PHONY:all

# all dont have Required directory, newest headers, finnaly output file
all:$(REQDIRS) $(DEPS) $(BUILDDIR)$(LIBNAME)

# build shared object library, depend on object files
$(BUILDDIR)$(LIBNAME):$(OBJS)
	$(CC) $(LDFLAGS) $(OFLAGS) $(BUILDDIR)$(LIBNAME) $(OBJS)

# compile sources to object files
$(OBJS):$(OBJDIR)%.o:$(SRCDIR)%.c $(DEPS)
	$(CC) $(CFLAGS) $(OFLAGS) $@ $<

.PHONY:prepare

prepare:$(REQDIRS)

$(REQDIRS):
	mkdir $@

.PHONY:install

install:$(BUILDDIR)$(LIBNAME)
	cp -fRp $(INCDIR)* $(INSTALLHDIR)
	cp -fp $(BUILDDIR)$(LIBNAME) $(INSTALLDIR)

.PHONY:clean

clean:
	rm -Rf $(OBJS) $(BUILDDIR)$(LIBNAME) $(OBJDIRS)
