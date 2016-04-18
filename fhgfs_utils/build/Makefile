# This is the beegfs_utils makefile.
#
# Use "make help" to find out about configuration options.


BEEGFS_COMMON_PATH ?= ../../beegfs_common/
BEEGFS_OPENTK_PATH ?= ../../beegfs_opentk_lib/
BEEGFS_THIRDPARTY_PATH ?= ../../beegfs_thirdparty/
BEEGFS_ONLINE_CFG_PATH ?= ../../beegfs_ctl
BEEGFS_FSCK_PATH ?= ../../beegfs_fsck
BEEGFS_FSCK_LEGACY_PATH ?= ../../beegfs_fsck_legacy
BEEGFS_JAVA_LIB_PATH ?= ../../beegfs_java_lib

ifneq ($(BEEGFS_VERSION),)
BEEGFS_EXTRA_FLAGS += 'BEEGFS_VERSION=$(BEEGFS_VERSION)'
endif

ifneq ($(BEEGFS_DEBUG),)
BEEGFS_EXTRA_FLAGS += 'BEEGFS_DEBUG=$(BEEGFS_DEBUG)'
endif


all: beegfs_ctl beegfs_fsck beegfs_fsck_legacy beegfs_java_lib

beegfs_ctl:
	$(MAKE) BEEGFS_COMMON_PATH=$(BEEGFS_COMMON_PATH) 	\
		BEEGFS_OPENTK_PATH=$(BEEGFS_OPENTK_PATH) 	\
		BEEGFS_THIRDPARTY_PATH=$(BEEGFS_THIRDPARTY_PATH) \
		$(EXTRA_FLAGS) $(BEEGFS_EXTRA_FLAGS) 	\
		-C $(BEEGFS_ONLINE_CFG_PATH)/build
	cp $(BEEGFS_ONLINE_CFG_PATH)/build/beegfs-ctl .
	
beegfs_fsck:
	$(MAKE) BEEGFS_COMMON_PATH=$(BEEGFS_COMMON_PATH) 	\
		BEEGFS_OPENTK_PATH=$(BEEGFS_OPENTK_PATH) 	\
		BEEGFS_THIRDPARTY_PATH=$(BEEGFS_THIRDPARTY_PATH) \
		$(EXTRA_FLAGS) $(BEEGFS_EXTRA_FLAGS) 	\
		-C $(BEEGFS_FSCK_PATH)/build
	cp $(BEEGFS_FSCK_PATH)/build/beegfs-fsck .

beegfs_fsck_legacy:
	$(MAKE) BEEGFS_COMMON_PATH=$(BEEGFS_COMMON_PATH) 	\
		BEEGFS_OPENTK_PATH=$(BEEGFS_OPENTK_PATH) 	\
		BEEGFS_THIRDPARTY_PATH=$(BEEGFS_THIRDPARTY_PATH) \
		$(EXTRA_FLAGS) $(BEEGFS_EXTRA_FLAGS) 	\
		-C $(BEEGFS_FSCK_LEGACY_PATH)/build
	cp $(BEEGFS_FSCK_LEGACY_PATH)/build/beegfs-fsck-legacy .

beegfs_java_lib:
	$(MAKE) -C $(BEEGFS_JAVA_LIB_PATH)/build
	cp $(BEEGFS_JAVA_LIB_PATH)/build/libjbeegfs.so $(BEEGFS_JAVA_LIB_PATH)/build/jbeegfs.jar .

clean:
	$(MAKE) -C $(BEEGFS_ONLINE_CFG_PATH)/build clean
	$(MAKE) -C $(BEEGFS_FSCK_PATH)/build clean
	$(MAKE) -C $(BEEGFS_FSCK_LEGACY_PATH)/build clean
#	$(MAKE) -C $(BEEGFS_JAVA_LIB_PATH)/build clean
	
	rm -f beegfs-ctl
	rm -f beegfs-fsck

rpm:
	./make-rpm

deb:
	./make-deb

help:
	@echo 'Optional Arguments:'
	@echo '   BEEGFS_DEBUG=1:'
	@echo '      Enables debug information and symbols.'
	@echo '   CXX=<compiler>:'
	@echo '      Specifies a c++ compiler.'
	@echo '   DISTCC=distcc:'
	@echo '      Enables the usage of distcc.'
	@echo '   BEEGFS_OPENTK_PATH=<path>:'
	@echo '      Path to the beegfs_opentk_lib directory.'
	@echo '   BEEGFS_COMMON_PATH=<path>:'
	@echo '      Path to the beegfs_common directory.'
	@echo '   BEEGFS_THIRDPARTY_PATH=<path>:'
	@echo '      Path to the beegfs_thirdparty directory.'
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

