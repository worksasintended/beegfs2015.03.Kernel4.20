# All detected features are included in "KERNEL_FEATURE_DETECTION"


########### OFED PART [START] ###########

ifeq ($(BEEGFS_OPENTK_IBVERBS),1)

ifneq ($(OFED_INCLUDE_PATH),)
OFED_DETECTION_PATH := $(OFED_INCLUDE_PATH)
else
OFED_DETECTION_PATH := ${KSRCDIR_PRUNED_HEAD}/include
endif


# OFED_HAS_RDMA_CREATE_QPTYPE Detection [START]
#
# Find out whether rdma_create_id function has qp_type argument.
# This is tricky because the function declaration spans multiple lines.
# Note: Was introduced in vanilla 3.0
OFED_HAS_RDMA_CREATE_QPTYPE = $(shell \
	if grep -A2 "struct rdma_cm_id \*rdma_create_id(" \
		${OFED_DETECTION_PATH}/rdma/rdma_cm.h 2>&1 | grep \
			"ib_qp_type qp_type);" 1>/dev/null 2>&1 ; \
	then echo "-DOFED_HAS_RDMA_CREATE_QPTYPE" ; \
	fi)
#
# OFED_HAS_RDMA_CREATE_QPTYPE Detection [END]

# OFED_HAS_SET_SERVICE_TYPE Detection [START]
#
# Find out whether rdma_set_service_type function has been declared.
# Note: Was introduced in vanilla 2.6.24
OFED_HAS_SET_SERVICE_TYPE = $(shell \
	if grep "rdma_set_service_type(struct rdma_cm_id \*id, int tos)" \
		${OFED_DETECTION_PATH}/rdma/rdma_cm.h 1>/dev/null 2>&1 ; \
	then echo "-DOFED_HAS_SET_SERVICE_TYPE" ; \
	fi)	
#
# OFED_HAS_SET_SERVICE_TYPE Detection [END]

endif # BEEGFS_OPENTK_IBVERBS

########### OFED PART [END] ###########


# KERNEL_HAS_SCSI_FC_COMPAT Detection [START]
#
# Find out whether the kernel has a scsi/fc_compat.h file, which defines
# vlan_dev_vlan_id.
# Note: We need this, because some kernels (e.g. RHEL 5.9's 2.6.18) forgot this
# include in their rdma headers, leading to implicit function declarations.
KERNEL_HAS_SCSI_FC_COMPAT = $(shell \
	if grep "vlan_dev_vlan_id" \
		${KSRCDIR_PRUNED_HEAD}/include/scsi/fc_compat.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_SCSI_FC_COMPAT" ; \
	fi)
#
# KERNEL_HAS_SCSI_FC_COMPAT Detection [END]




# Combine results
# (We might be called from the wrapping client build, so we use "+=" here.) 
KERNEL_FEATURE_DETECTION += $(OFED_HAS_RDMA_CREATE_QPTYPE)
KERNEL_FEATURE_DETECTION += $(OFED_HAS_SET_SERVICE_TYPE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_SCSI_FC_COMPAT)
