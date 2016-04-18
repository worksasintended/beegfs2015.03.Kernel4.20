# This is the beegfs_common makefile.
# It creates a static library.
# 
# Use "make help" to find out about configuration options.

BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_THIRDPARTY_PATH ?= ../../beegfs_thirdparty/
BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_AUTO_CXX11_SCRIPT_PATH ?= ./check-cxx-for-c++11.sh

CPPUNIT_INC_PATH ?= $(BEEGFS_THIRDPARTY_PATH)/source/cppunit_install/usr/local/include


LIB = beegfs-common

CXX = g++
CXXFLAGS = -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	-I../source -I$(BEEGFS_OPENTK_PATH)/include -I$(CPPUNIT_INC_PATH) \
	-pthread -Wall -fmessage-length=0 -fno-strict-aliasing \
	-Wunused-variable -Woverloaded-virtual -Wextra -Wno-unused-parameter \
	-ggdb3 $(USER_CXXFLAGS)
CXXFLAGS_RELEASE = -O3 -Wuninitialized
CXXFLAGS_DEBUG = -O0 -ggdb3 -D_FORTIFY_SOURCE=2 \
	-DBEEGFS_DEBUG -DDEBUG_READWRITE -DDEBUG_MUTEX_LOCKING -DDEBUG_REFCOUNT \
	-DDEBUG_BACKTRACE -DLOG_DEBUG_MESSAGES
LDFLAGS = -pthread -rdynamic $(USER_LDFLAGS)
LDFLAGS_RELASE = -O3
LDFLAGS_DEBUG = -O0

SOURCES = $(shell find ../source -name '*.cpp')

OBJECTS = $(SOURCES:.cpp=.o)
CLEANUP_OBJECTS = $(shell find ../source -name '*.o')

DEPENDENCY_FILES = $(shell find ../source -name '*.d')

# build source files for shared lib beegfs-deeper.so
ifeq ($(BEEGFS_DEEPER_SO),1)
CXXFLAGS += -fPIC
LDFLAGS = -static -pthread -rdynamic $(USER_LDFLAGS)
endif

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

# HSM
ifeq ($(BEEGFS_HSM),1)
CXXFLAGS += -DBEEGFS_HSM
endif

# automatic selection of c++11 if compiler is g++ with version >=4.8
AUTO_CXX11 ?= $(shell $(BEEGFS_AUTO_CXX11_SCRIPT_PATH) $(CXX) )
ifeq ($(AUTO_CXX11),1)
CXXFLAGS += -std=c++11
endif


all: $(SOURCES) $(LIB)

all-deeper: $(SOURCES) deeper

deeper: $(OBJECTS)
	ar -crs lib$(LIB)-unstripped.a $(OBJECTS)

	echo "CREATE libbeegfs-common-opentk-unstripped.a" > arlib-unstripped.in
	echo "ADDLIB lib$(LIB)-unstripped.a" >> arlib-unstripped.in
	echo "ADDLIB $(BEEGFS_OPENTK_PATH)/build/libbeegfs-opentk-unstripped.a" >> arlib-unstripped.in
	echo "SAVE" >> arlib-unstripped.in
	echo "END" >> arlib-unstripped.in
	ar -M < arlib-unstripped.in
	ranlib lib$(LIB)-unstripped.a

	cp lib$(LIB)-unstripped.a lib$(LIB).a
	${STRIP} --strip-debug lib$(LIB).a

	echo "CREATE libbeegfs-common-opentk.a" > arlib.in
	echo "ADDLIB lib$(LIB).a" >> arlib.in
	echo "ADDLIB $(BEEGFS_OPENTK_PATH)/build/libbeegfs-opentk.a" >> arlib.in
	echo "SAVE" >> arlib.in
	echo "END" >> arlib.in
	ar -M < arlib.in
	ranlib lib$(LIB).a

	${STRIP} --strip-debug libbeegfs-common-opentk.a


$(LIB): $(OBJECTS)
	ar -r lib$(LIB).a $(OBJECTS)

.cpp.o: 
	$(CXX) $(CXXFLAGS) -c $(@:.o=.cpp) -E -MMD -MF$(@:.o=.d) -MT$(@) -o/dev/null
	$(DISTCC) $(CXX) $(CXXFLAGS) -o$(@) -c $(@:.o=.cpp) 

clean:
	rm -rf $(CLEANUP_OBJECTS) $(DEPENDENCY_FILES) lib$(LIB).a lib$(LIB)-unstripped.a \
		libbeegfs-common-opentk.a libbeegfs-common-opentk-unstripped.a arlib.in arlib-unstripped.in

help:
	@echo 'Optional Arguments:'
	@echo '- BEEGFS_DEBUG=1:'
	@echo '	  Enables debug information and symbols.'
	@echo '- CXX=<compiler>:'
	@echo '    Specifies a c++ compiler.'
	@echo '- DISTCC=distcc:'
	@echo '    Enables the usage of distcc.'

# Include dependency files
ifneq ($(DEPENDENCY_FILES),)
include $(DEPENDENCY_FILES)
endif

