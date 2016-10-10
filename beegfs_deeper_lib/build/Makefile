# This is the beegfs_deeper_lib makefile.
# It creates a shared library.
# 
# Use "make help" to find out about configuration options.


LIB = beegfs-deeper

# rebuild common lib for shared lib
export BEEGFS_DEEPER_SO = 1

# need to change to beegfs, when the packages are renamed
BEEGFS_VERSION_PATH ?= ../../beegfs-version
BEEGFS_COMMON_PATH ?= ../../beegfs_common/
BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_THIRDPARTY_PATH ?= ../../beegfs_thirdparty/

PREFIX                ?= /opt/beegfs
ETC_DIR               ?= /etc


CXX = g++
CXXFLAGS = -D_FILE_OFFSET_BITS=64 -I../source -I../include \
	-I$(BEEGFS_OPENTK_PATH)/include -I$(BEEGFS_COMMON_PATH)/source \
	-Wall -fmessage-length=0 -fno-strict-aliasing -Wunused-variable \
	-fPIC -Wextra -Wno-unused-parameter -pthread -ggdb3 $(USER_CXXFLAGS)
CXXFLAGS_RELEASE = -O3 -Wuninitialized
CXXFLAGS_DEBUG = -O1 -ggdb3 -D_FORTIFY_SOURCE=2 -DBEEGFS_DEBUG
LDFLAGS = -L$(BEEGFS_COMMON_PATH)/build -static -pthread \
	-lbeegfs-common-opentk -lz -ldl -rdynamic $(USER_LDFLAGS)
LDFLAGS_SHARED = -L$(BEEGFS_COMMON_PATH)/build -L$(BEEGFS_THIRDPARTY_PATH)/build \
	-shared -pthread -Wl,-whole-archive -lbeegfs-common-opentk -lcppunit \
	-Wl,-no-whole-archive -ldl -rdynamic $(USER_LDFLAGS)
LDFLAGS_RELASE = -O3
LDFLAGS_DEBUG = -O0

SOURCES_CPP = $(shell find ../source -name '*.cpp')
SOURCES_C = $(shell find ../source -name '*.c')

OBJECTS_CPP = $(SOURCES_CPP:.cpp=.o)
OBJECTS_C = $(SOURCES_C:.c=.o)

CLEANUP_OBJECTS = $(shell find ../source -name '*.o')

DEPENDENCY_FILES = $(shell find ../source -name '*.d')

ifeq ($(BEEGFS_DEBUG),)
CXXFLAGS += $(CXXFLAGS_RELEASE)
LDFLAGS += $(LDFLAGS_RELASE)
LDFLAGS_SHARED += $(LDFLAGS_RELASE)
else
CXXFLAGS += $(CXXFLAGS_DEBUG)
LDFLAGS += $(LDFLAGS_DEBUG)
LDFLAGS_SHARED += $(LDFLAGS_DEBUG)
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


all: test_version $(SOURCES_CPP) $(SOURCES_C) $(LIB)

test_version:
	@# just call the beegfs-version script to check whether it returns an error. the script output is
	@# used in other places of this makefile and we want to make sure that we don't mess up version
	@# tags in the binary with script errors.
	@echo "beegfs version check..."
	@$(BEEGFS_VERSION_PATH) --verbose

$(LIB): $(OBJECTS_CPP) $(OBJECTS_C) libshared

strip:
	cp -a lib$(LIB).a lib$(LIB)-unstripped.a
	${STRIP} --strip-debug lib$(LIB).a
	cp -a lib$(LIB).so lib$(LIB)-unstripped.so
	${STRIP} --strip-debug lib$(LIB).so

libthirdparty:
	$(MAKE) CXXFLAGS='-fPIC' -C $(BEEGFS_THIRDPARTY_PATH)/build/ clean_cppunit
	$(MAKE) CXXFLAGS='-fPIC' -C $(BEEGFS_THIRDPARTY_PATH)/build/ cppunit

libopentk:
	$(MAKE) -C $(BEEGFS_OPENTK_PATH)/build/ all-deeper

libcommon: libopentk libthirdparty
	$(MAKE) -C $(BEEGFS_COMMON_PATH)/build/ clean
	$(MAKE) -C $(BEEGFS_COMMON_PATH)/build/ all-deeper

libstatic: libcommon
	ar -crs libbeegfs-cache.a $(OBJECTS_CPP) $(OBJECTS_C)
	echo "CREATE lib$(LIB).a" > arlib.in
	echo "ADDLIB libbeegfs-cache.a" >> arlib.in
	echo "ADDLIB $(BEEGFS_COMMON_PATH)/build/libbeegfs-common-opentk.a"  >> arlib.in
	echo "ADDLIB $(BEEGFS_THIRDPARTY_PATH)/build/libcppunit.a"  >> arlib.in
	echo "SAVE" >> arlib.in
	echo "END" >> arlib.in
	ar -M < arlib.in
	ranlib lib$(LIB).a



libshared: libstatic
	$(CXX) $(OBJECTS_CPP) $(OBJECTS_C) $(LDFLAGS_SHARED) -o lib$(LIB).so


# build
package_lib: 

	# clean up old stuff first
	$(MAKE) clean

	$(MAKE)

.cpp.o: 
	$(CXX) $(CXXFLAGS) -c $(@:.o=.cpp) -E -MMD -MF$(@:.o=.d) -MT$(@) -o/dev/null
	$(DISTCC) $(CXX) $(CXXFLAGS) -o $@ -c $(@:.o=.cpp) 

.c.o: 
	$(CXX) $(CXXFLAGS) -c $(@:.o=.c) -E -MMD -MF$(@:.o=.d) -MT$(@) -o/dev/null
	$(DISTCC) $(CXX) $(CXXFLAGS) -o $@ -c $(@:.o=.c) 

install: package_lib
	install -D -m 755 lib$(LIB).so $(DESTDIR)/$(PREFIX)/lib/lib$(LIB).so

package_install: install
	@# PREFIX is something like /usr or /usr/local or /opt/beegfs
	@# DESTDIR is mainly for package builders
	install -D -m 644 dist/etc/ld.so.conf.d/beegfs-deeper.conf.in $(DESTDIR)/$(ETC_DIR)/ld.so.conf.d/beegfs-deeper.conf
	install -D -m 644 dist/etc/beegfs/beegfs-deeper-lib.conf.in $(DESTDIR)/$(ETC_DIR)/beegfs/beegfs-deeper-lib.conf
	sed -i -e "s#__PREFIX__#$(PREFIX)#" $(DESTDIR)/$(ETC_DIR)/ld.so.conf.d/beegfs-deeper.conf
	mkdir -p ${RPM_BUILD_ROOT}/usr/include
	cp -a ../include/deeper ${RPM_BUILD_ROOT}/usr/include/

clean:
	rm -rf $(CLEANUP_OBJECTS) $(DEPENDENCY_FILES) lib$(LIB).so \
		lib$(LIB)-unstripped.so lib$(LIB).a lib$(LIB)-unstripped.a \
		libbeegfs-cache.a arlib.in
	$(MAKE) -C $(BEEGFS_COMMON_PATH)/build/ clean
	$(MAKE) -C $(BEEGFS_OPENTK_PATH)/build/ clean
	$(MAKE) -C $(BEEGFS_THIRDPARTY_PATH)/build/ clean_cppunit

rpm:
	./make-rpm

deb:
	./make-deb

help:
	@echo 'Optional Arguments:'
	@echo '   BEEGFS_DEBUG=1:'
	@echo '      Enables debug information and symbols.'
	@echo '   BEEGFS_THIRDPARTY_PATH=<path>:'
	@echo '      Path to the beegfs_thirdparty directory.'
	@echo '   BEEGFS_COMMON_PATH=<path>:'
	@echo '      Path to the beegfs_common directory.'
	@echo '   BEEGFS_OPENTK_PATH=<path>:'
	@echo '      Path to the beegfs_opentk directory.'
	@echo '   CXX=<compiler>:'
	@echo '      Specifies a C++ compiler.'
	@echo '   DISTCC=distcc:'
	@echo '      Enables the usage of distcc.'
	@echo '   PREFIX=/some/path (default: /opt/beegfs)'
	@echo '      Installation root directory.'
	@echo '   ETC_DIR=<path>:'
	@echo '      Prefix for configuration files (default: /etc).'
	@echo '   DESTDIR=/some/path:'
	@echo '      Mainly for package building, used as primary path prefix then.'
	@echo
	@echo 'targets:'
	@echo '   all (default)  - compile the code and create the library.'
	@echo '   install        - install the library and config files.'
	@echo '   clean          - delete compiled files.'
	@echo '   rpm            - build an rpm package file'
	@echo '   deb            - build debian package files'
	@echo '   help           - print this help text.'

# Include dependency files
ifneq ($(DEPENDENCY_FILES),)
include $(DEPENDENCY_FILES)
endif

