# This is the beegfs_common makefile.
# 
# Use "make help" to find out about configuration options.


BEEGFS_COMMON_PATH ?= ../../beegfs_common/
BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_ONLINE_CFG_PATH ?= ../../beegfs_ctl
BEEGFS_FSCK_PATH ?= ../../beegfs_fsck

ifneq ($(BEEGFS_VERSION),)
BEEGFS_EXTRA_FLAGS += 'BEEGFS_VERSION="$(BEEGFS_VERSION)"'
endif

ifneq ($(BEEGFS_DEBUG),)
BEEGFS_EXTRA_FLAGS += 'BEEGFS_DEBUG=$(BEEGFS_DEBUG)'
endif


all: 

clean: 

rpm:
	./make-rpm

deb:
	./make-deb

help:
	@echo 'Optional Arguments:'
	@echo '   BEEGFS_DEBUG=1:'
	@echo '	     Enables debug information and symbols.'
	@echo '   CXX=<compiler>:'
	@echo '      Specifies a c++ compiler.'
	@echo '   DISTCC=distcc:'
	@echo '      Enables the usage of distcc.'
	@echo '   BEEGFS_OPENTK_PATH=<path>:'
	@echo '      Path to the beegfs_opentk_lib directory.'
	@echo '   BEEGFS_COMMON_PATH=<path>:'
	@echo '      Path to the beegfs_common directory.'
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

