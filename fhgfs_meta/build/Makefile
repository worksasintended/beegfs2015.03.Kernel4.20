# This is the beegfs_meta makefile.
# It creates an ELF executable.
# 
# Use "make help" to find out about configuration options.


EXE = beegfs-meta

BEEGFS_VERSION_PATH    ?= ../../beegfs-version
BEEGFS_COMMON_PATH ?= ../../beegfs_common/
BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_THIRDPARTY_PATH ?= ../../beegfs_thirdparty/
BEEGFS_AUTO_CXX11_SCRIPT_PATH ?= $(BEEGFS_COMMON_PATH)/build/check-cxx-for-c++11.sh
CPPUNIT_INC_PATH ?= $(BEEGFS_THIRDPARTY_PATH)/source/cppunit_install/usr/local/include

CXX = g++
CXXFLAGS = -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	-I../source -I$(BEEGFS_OPENTK_PATH)/include -I$(BEEGFS_COMMON_PATH)/source \
	-I$(CPPUNIT_INC_PATH) \
	-pthread -Wall -fmessage-length=0 -fno-strict-aliasing \
	-Wunused-variable -Woverloaded-virtual -Wextra -Wno-unused-parameter \
	-ggdb3 $(USER_CXXFLAGS)
CXXFLAGS_RELEASE = -O3 -Wuninitialized
CXXFLAGS_DEBUG = -O1 -ggdb3 -D_FORTIFY_SOURCE=2 \
	-DBEEGFS_DEBUG -DDEBUG_READWRITE -DDEBUG_MUTEX_LOCKING -DDEBUG_REFCOUNT \
	-DDEBUG_BACKTRACE -DLOG_DEBUG_MESSAGES

# enable internal debug code, but compile with optimizations 
CXXFLAGS_DEBUG_WITH_OPT = -O3 -Wuninitialized -ggdb3 -D_FORTIFY_SOURCE=2 \
	-DBEEGFS_DEBUG -DDEBUG_READWRITE -DDEBUG_MUTEX_LOCKING -DDEBUG_REFCOUNT \
	-DDEBUG_BACKTRACE -DLOG_DEBUG_MESSAGES

# enable special reference DirInode debug code
CXXFLAG_DEBUG_RELEASE_DIR = -DBEEGFS_DEBUG_RELEASE_DIR

THIRTPARTY_LIBS = $(BEEGFS_THIRDPARTY_PATH)/build/libcppunit.a

LDFLAGS = -L$(BEEGFS_COMMON_PATH)/build -L$(BEEGFS_OPENTK_PATH)/build \
	-L$(BEEGFS_THIRDPARTY_PATH)/build -pthread -ldl -lbeegfs-common -lbeegfs-opentk -lrt \
	$(THIRTPARTY_LIBS) -rdynamic $(USER_LDFLAGS)
LDFLAGS_RELASE = -O3
LDFLAGS_DEBUG = -O0

SOURCES = $(shell find ../source -name '*.cpp')

OBJECTS = $(SOURCES:.cpp=.o)
CLEANUP_OBJECTS = $(shell find ../source -name '*.o')

DEPENDENCY_FILES = $(shell find ../source -name '*.d')


# set compiler/linker flags for chosen build type...

ifeq ($(BEEGFS_DEBUG_OPT), 1)             # debug code with optimizations
   CXXFLAGS += $(CXXFLAGS_DEBUG_WITH_OPT)
   LDFLAGS += $(LDFLAGS_RELASE)
else
ifeq ($(BEEGFS_DEBUG),1)                  # debug code without optimizations
   CXXFLAGS += $(CXXFLAGS_DEBUG)
   LDFLAGS += $(LDFLAGS_DEBUG)
   ifeq ($(BEEGFS_DEBUG_RELEASE_DIR), 1)     # extra release dir debugging
      CXXFLAGS += $(CXXFLAG_DEBUG_RELEASE_DIR)
   endif
else                                     # release code / no debug options
   CXXFLAGS += $(CXXFLAGS_RELEASE)
   LDFLAGS += $(LDFLAGS_RELASE)
endif # BEEGFS_DEBUG
endif # BEEGFS_DEBUG_OPT

# HSM
ifeq ($(BEEGFS_HSM),1)
   CXXFLAGS += -DBEEGFS_HSM
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


all: test_version $(SOURCES) $(EXE)

test_version:
	@# just call the beegfs-version script to check whether it returns an error. the script output is
	@# used in other places of this makefile and we want to make sure that we don't mess up version
	@# tags in the binary with script errors.
	@echo "beegfs version check..."
	@$(BEEGFS_VERSION_PATH) --verbose

$(EXE): $(OBJECTS) $(BEEGFS_COMMON_PATH)/build/libbeegfs-common.a \
		$(BEEGFS_OPENTK_PATH)/build/libbeegfs-opentk.so
	$(CXX) -o $(EXE) $(OBJECTS) $(LDFLAGS)

strip:
	# we don't strip the $(EXE) target by default due to debug packages
	cp -a $(EXE) $(EXE)-unstripped
	${STRIP} --strip-debug $(EXE)

.cpp.o: 
	$(CXX) $(CXXFLAGS) -c $(@:.o=.cpp) -E -MMD -MF$(@:.o=.d) -MT$(@) -o/dev/null
	$(DISTCC) $(CXX) $(CXXFLAGS) -o$(@) -c $(@:.o=.cpp) 

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
	@echo '   BEEGFS_DEBUG_RELEASE_DIR:'
	@echo '     Additional to BEEGFS_DEBUG=1, enables further DirInode reference debug code.'
	@echo '   BEEGFS_DEBUG_OPT=1:'
	@echo '     Enables internal debug code, but compiled with optimizations.'
	@echo '   CXX=<compiler>:'
	@echo '      Specifies a c++ compiler.'
	@echo '   DISTCC=distcc:'
	@echo '      Enables the usage of distcc.'
	@echo '   BEEGFS_OPENTK_PATH=<path>:'
	@echo '      Path to the beegfs_opentk_lib directory.'
	@echo '   BEEGFS_COMMON_PATH=<path>:'
	@echo '      Path to the beegfs_common directory.'
	@echo '   BEEGFS_THIRDPARTY_PATH=<path>:'
	@echo '       Path to the beegfs_thirdparty directory.'	
	@echo
	@echo 'Targets:'
	@echo '   all (default)     - build only'
	@echo '   prepare_release   - build and copy files into the RELEASE_PATH directory'
	@echo '   rpm               - create a rpm file and write it to buildroot/RPMS/'
	@echo '   deb               - create debian package file'
	@echo '   help              - print this help message'


# Include dependency files
ifneq ($(DEPENDENCY_FILES),)
include $(DEPENDENCY_FILES)
endif

