# This is the beegfs_admon makefile.
# It creates an ELF executable.
# 
# Use "make help" to find out about configuration options.

SHELL := /bin/bash

EXE = beegfs-admon

BEEGFS_VERSION_PATH    ?= ../../beegfs-version
BEEGFS_COMMON_PATH ?= ../../beegfs_common/
BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_THIRDPARTY_PATH ?= ../../beegfs_thirdparty/
ADMON_GUI_PATH ?= ../../beegfs_admon_gui/
BEEGFS_AUTO_CXX11_SCRIPT_PATH ?= $(BEEGFS_COMMON_PATH)/build/check-cxx-for-c++11.sh

SQLITE_INC_PATH  ?= $(BEEGFS_THIRDPARTY_PATH)/source/sqlite
OPENSSL_INC_PATH ?= $(BEEGFS_THIRDPARTY_PATH)/source/openssl/include
CPPUNIT_INC_PATH ?= $(BEEGFS_THIRDPARTY_PATH)/source/cppunit_install/usr/local/include
TICPP_INC_PATH  ?= $(BEEGFS_THIRDPARTY_PATH)/source/ticpp
MONGOOSE_INC_PATH  ?= $(BEEGFS_THIRDPARTY_PATH)/source/mongoose

ANT_FINDBUGS_LIB ?= $(shell cd $(BEEGFS_THIRDPARTY_PATH) && pwd)/source/findbugs/lib/findbugs-ant.jar

# JAVA_HOME will be honoured automatically by 'ant'

CXX = g++
CC = gcc
CXXFLAGS = -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DTIXML_USE_TICPP \
	-I../source -I$(BEEGFS_OPENTK_PATH)/include -I$(BEEGFS_COMMON_PATH)/source \
	-isystem $(SQLITE_INC_PATH) -isystem $(OPENSSL_INC_PATH) \
	-isystem $(CPPUNIT_INC_PATH) -isystem $(TICPP_INC_PATH) \
	-isystem $(MONGOOSE_INC_PATH) -pthread -Wall -fmessage-length=0 \
	-Wunused-variable -Woverloaded-virtual -Wextra -Wno-unused-parameter \
	-ggdb3 $(USER_CXXFLAGS)
CXXFLAGS_RELEASE = -O3 -Wuninitialized
CXXFLAGS_DEBUG = -O1 -ggdb3 -D_FORTIFY_SOURCE=2 \
	-DBEEGFS_DEBUG -DDEBUG_READWRITE -DDEBUG_MUTEX_LOCKING -DDEBUG_REFCOUNT \
	-DDEBUG_BACKTRACE  -DLOG_DEBUG_MESSAGES

THIRDPARTY_LIBS = $(BEEGFS_THIRDPARTY_PATH)/build/libcppunit.a \
		  $(BEEGFS_THIRDPARTY_PATH)/build/libsqlite3.a \
		  $(BEEGFS_THIRDPARTY_PATH)/build/libssl.a     \
		  $(BEEGFS_THIRDPARTY_PATH)/build/libcrypto.a  \
		  $(BEEGFS_THIRDPARTY_PATH)/build/libticpp.a   \
		  $(BEEGFS_THIRDPARTY_PATH)/build/libmongoose.a

BEEGFS_OPENTK = $(BEEGFS_OPENTK_PATH)/build/libbeegfs-opentk.a

LDFLAGS = -L$(BEEGFS_COMMON_PATH)/build \
	-pthread -lrt -lbeegfs-common $(BEEGFS_OPENTK) \
	$(THIRDPARTY_LIBS) -rdynamic $(USER_LDFLAGS)

LDFLAGS_RELASE =-O3 -ldl
LDFLAGS_DEBUG =-O0 -ldl

SOURCES = $(shell find ../source -name '*.cpp' -o -name '*.c')

OBJECTS = $(SOURCES:.cpp=.o)
CLEANUP_OBJECTS = $(shell find ../source -name '*.o')

DEPENDENCY_FILES = $(shell find ../source -name '*.d')

ifeq ($(BEEGFS_DEBUG),)
CXXFLAGS += $(CXXFLAGS_RELEASE)
LDFLAGS += $(LDFLAGS_RELASE)
else
CXXFLAGS += $(CXXFLAGS_DEBUG)
LDFLAGS += $(LDFLAGS_DEBUG)
endif

# if path to strip command was not given, use default
# (alternative strip is important when cross-compiling)
ifeq ($(STRIP),)
STRIP=strip
endif

# beegfs version
# "override", because we don't want BEEGFS_VERSION(_CODE) from the environment.
# VER and RELEASE_STR are the variables that can be set by the environment.
override BEEGFS_VERSION := $(shell bash -c \
	"VER=$(VER) RELEASE_STR=$(RELEASE_STR) $(BEEGFS_VERSION_PATH) --print_version")
override BEEGFS_VERSION_CODE := $(shell bash -c \
	"VER=$(VER) RELEASE_STR=$(RELEASE_STR) $(BEEGFS_VERSION_PATH) --print_version_code")
CXXFLAGS += -DBEEGFS_VERSION=\"$(BEEGFS_VERSION)\" -DBEEGFS_VERSION_CODE=$(BEEGFS_VERSION_CODE)

# automatic selection of c++11 if compiler is g++ with version >=4.8
AUTO_CXX11 ?= $(shell $(BEEGFS_AUTO_CXX11_SCRIPT_PATH) $(CXX) )
ifeq ($(AUTO_CXX11),1)
CXXFLAGS += -std=c++11
endif


all: admon

admon: test_version $(SOURCES) $(EXE)

admon_gui: test_version
	set -e 										&& \
	pushd $(ADMON_GUI_PATH)								&& \
	ant -lib $(ANT_FINDBUGS_LIB) -DBEEGFS_VERSION=$(BEEGFS_VERSION) clean || true	&& \
	ant -lib $(ANT_FINDBUGS_LIB) -DBEEGFS_VERSION=$(BEEGFS_VERSION)			&& \
	popd # ADMON_GUI_PATH

test_version:
	@# just call the beegfs-version script to check whether it returns an error. the script output is
	@# used in other places of this makefile and we want to make sure that we don't mess up version
	@# tags in the binary with script errors.
	@echo "beegfs version check..."
	@$(BEEGFS_VERSION_PATH) --verbose

$(EXE): libs $(OBJECTS)
	$(CXX) -o $(EXE) $(OBJECTS) $(LDFLAGS)

strip:
	cp -a $(EXE) $(EXE)-unstripped
	${STRIP} --strip-debug $(EXE)

.cpp.o: libs
	$(CXX) $(CXXFLAGS) -c $(@:.o=.cpp) -E -MMD -MF$(@:.o=.d) -MT$(@) -o/dev/null
	$(DISTCC) $(CXX) $(CXXFLAGS) -o$(@) -c $(@:.o=.cpp) 

sqlite:     
	pushd $(BEEGFS_THIRDPARTY_PATH)/build && \
	$(MAKE) sqlite && \
	popd

openssl:
	pushd $(BEEGFS_THIRDPARTY_PATH)/build && \
	$(MAKE) openssl && \
	popd
	
mongoose:
	pushd $(BEEGFS_THIRDPARTY_PATH)/build && \
	$(MAKE) mongoose && \
	popd
	
ticpp:
	pushd $(BEEGFS_THIRDPARTY_PATH)/build && \
	$(MAKE) ticpp && \
	popd

libs: openssl sqlite mongoose ticpp

clean:
	rm -rf $(CLEANUP_OBJECTS) $(DEPENDENCY_FILES) $(EXE) $(EXE)-unstripped

rpm:
	./make-rpm

deb:
	./make-deb

help:
	@echo 'Optional Arguments:'
	@echo '   BEEGFS_DEBUG=1:'
	@echo '	    Enables debug information and symbols.'
	@echo '   CXX=<compiler>:'
	@echo '     Specifies a c++ compiler.'
	@echo '   DISTCC=distcc:'
	@echo '     Enables the usage of distcc.'
	@echo '   BEEGFS_COMMON_PATH=<path>:'
	@echo '       Path to the beegfs_common directory.'
	@echo '   BEEGFS_OPENTK_PATH=<path>:'
	@echo '       Path to the beegfs_opentk_lib directory.'
	@echo '   BEEGFS_THIRDPARTY_PATH=<path>:'
	@echo '       Path to the beegfs_thirdparty directory.'	
	@echo '   JAVA_HOME=<path>'
	@echo '       Path to Java JDK installation (optional)'
	@echo '   ADMON_GUI_PATH=<path>'
	@echo '      Path to the beegfs_admon_gui directory'
	@echo 'targets:'
	@echo '   all (default) - compile the code'
	@echo '   rpm           - create an rpm package'
	@echo '   deb           - create debian package file'
	@echo '   clean         - delete previously compiled files'
	@echo '   help          - print this help'
	@echo


# Include dependency files
ifneq ($(DEPENDENCY_FILES),)
include $(DEPENDENCY_FILES)
endif

