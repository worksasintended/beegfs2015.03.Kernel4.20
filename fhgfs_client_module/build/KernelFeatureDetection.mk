# All detected features are included in "KERNEL_FEATURE_DETECTION"

# parameters:
# $1: name to define when grep finds something
# $2: grep flags and expression
# $3: input files in linux source tree
define define_if_matches
$(eval \
   KERNEL_FEATURE_DETECTION += $$(shell \
      grep -q -s $2 $(addprefix ${KSRCDIR_PRUNED_HEAD}/include/linux/,$3) \
         && echo "-D$(strip $1)"))
endef

# KERNEL_HAS_DROP_NLINK Detection [START]
#
# kernels >=v4.4 expect a netns argument for rdma_create_id
KERNEL_FEATURE_DETECTION += $(shell \
   grep -qs "struct rdma_cm_id \*rdma_create_id.struct net \*net," \
      ${OFED_DETECTION_PATH}/rdma/rdma_cm.h \
      && echo "-DOFED_HAS_NETNS")

# kernels >=v4.4 split up ib_send_wr into a lot of other structs
KERNEL_FEATURE_DETECTION += $(shell \
   grep -qs -F "struct ib_atomic_wr {" \
      ${OFED_DETECTION_PATH}/rdma/ib_verbs.h \
      && echo "-DOFED_SPLIT_WR")

# Find out whether the kernel has a drop_nlink function.
# Note: Was added in vanilla 2.6.19, but RedHat adds it to 2.6.18.
KERNEL_HAS_DROP_NLINK = $(shell \
	if grep "drop_nlink(struct inode \*inode)" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_DROP_NLINK" ; \
	fi)
#
# KERNEL_HAS_DROP_NLINK Detection [END]

# KERNEL_HAS_CLEAR_NLINK Detection [START]
#
# Find out whether the kernel has a clear_nlink function.
# Note: Was added in vanilla 2.6.19, but RedHat adds it to 2.6.18.
KERNEL_HAS_CLEAR_NLINK = $(shell \
	if grep "clear_nlink(struct inode \*inode)" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_CLEAR_NLINK" ; \
	fi)
#
# KERNEL_HAS_CLEAR_NLINK Detection [END]

# KERNEL_HAS_WBC_NONRWRITEINDEXUPDATE Detection [START]
#
# Find out whether the kernel has a no_nrwrite_index_update field in struct
# writeback_control.
# Note: Existed in vanilla 2.6.28-2.6.34, but RedHat removes it from their 
# 2.6.32.
KERNEL_HAS_WBC_NONRWRITEINDEXUPDATE = $(shell \
	if grep "unsigned no_nrwrite_index_update" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/writeback.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_WRITEBACK_NONRWRITEINDEXUPDATE" ; \
	fi)
#
# KERNEL_HAS_WBC_NONRWRITEINDEXUPDATE Detection [END]


# KERNEL_HAS_TEST_LOCK_VOID_TWOARGS Detection [START]
#
# Find out whether kernel has posix_test_lock method with two args and void
# return value.
# Note: There were at least three different versions since 2.6.16 before this
# was introduced in vanilla 2.6.23
KERNEL_HAS_TEST_LOCK_VOID_TWOARGS = $(shell \
	if grep "void posix_test_lock(struct file \*, struct file_lock \*);" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_TEST_LOCK_VOID_TWOARGS" ; \
	fi)
#
# KERNEL_HAS_TEST_LOCK_VOID_TWOARGS Detection [END]


# KERNEL_HAS_ATTR_OPEN Detection [START]
#
# Find out whether kernel defines ATTR_OPEN.
# Note: Was introduced in vanilla 2.6.24
KERNEL_HAS_ATTR_OPEN = $(shell \
	if grep "define ATTR_OPEN" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_ATTR_OPEN" ; \
	fi)
#
# KERNEL_HAS_ATTR_OPEN Detection [END]

# KERNEL_HAS_IHOLD Detection [START]
#
# Find out whether the kernel has a ihold function.
# Note: Was added in vanilla 2.6.37, but RedHat adds it to their 2.6.32.
KERNEL_HAS_IHOLD = $(shell \
	if grep ' ihold(struct inode.*)' \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_IHOLD" ; \
	fi)
#
# KERNEL_HAS_IHOLD Detection [END]

# KERNEL_HAS_TASK_IO_ACCOUNTING Detection [START]
#
# Find out whether the kernel has task_io_account_read/write.
# Note: Was introduced in vanilla 2.6.20, but RedHat adds it to 2.6.18.
KERNEL_HAS_TASK_IO_ACCOUNTING = $(shell \
	if grep ' task_io_account_read(size_t .*)' \
		${KSRCDIR_PRUNED_HEAD}/include/linux/task_io_accounting_ops.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_TASK_IO_ACCOUNTING" ; \
	fi)
#
# KERNEL_HAS_TASK_IO_ACCOUNTING Detection [END]

# KERNEL_HAS_FSYNC_RANGE Detection [START]
#
# Find out whether the kernel has fsync start and end range arguments.
# Note: fsync start and end were added in vanilla 3.1, but SLES11SP3 adds it to 
#       its 3.0 kernel.
KERNEL_HAS_FSYNC_RANGE = $(shell \
	if grep "int (\*fsync) (struct file \*, loff_t, loff_t, int datasync);" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_FSYNC_RANGE" ; \
	fi)
#
# KERNEL_HAS_FSYNC_RANGE Detection [END]

# KERNEL_HAS_INC_NLINK Detection [START]
#
# Find out whether the kernel has inc_nlink()
# Note: inc_nlink was added in vanilla 2.6.19, but RHEL5 also has it.
KERNEL_HAS_INC_NLINK = $(shell \
	if grep " inc_nlink(struct inode.*)" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_INC_NLINK" ; \
	fi)
#
# KERNEL_HAS_INC_NLINK Detection [END]

# KERNEL_HAS_S_D_OP Detection [START]
#
# Find out whether the kernel has define struct dentry_operations *s_d_op
# in struct super_block. If it has it used that to check if the file system
# needs to revalidate dentries.
#
KERNEL_HAS_S_D_OP = $(shell \
	if grep "struct dentry_operations \*s_d_op;" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_S_D_OP" ; \
	fi)
# KERNEL_HAS_S_D_OP Detection [END]

# KERNEL_HAS_LOOKUP_CONTINUE Detection [START]
#
# Find out whether the kernel has define struct dentry_operations *s_d_op
# in struct super_block. If it has it used that to check if the file system
# needs to revalidate dentries.
#
KERNEL_HAS_LOOKUP_CONTINUE = $(shell \
	if grep "define LOOKUP_CONTINUE " \
		${KSRCDIR_PRUNED_HEAD}/include/linux/namei.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_LOOKUP_CONTINUE" ; \
	fi)
# KERNEL_HAS_LOOKUP_CONTINUE Detection [END]

# KERNEL_HAS_D_MATERIALISE_UNIQUE Detection [START]
#
# Find out whether the kernel has d_materialise_unique() to
# add dir dentries.
#
# Note: d_materialise_unique was added in vanilla 2.6.19 (backported to rhel5
# 2.6.18) and got merged into d_splice_alias in vanilla 3.19. 
#
KERNEL_HAS_D_MATERIALISE_UNIQUE = $(shell \
	if grep "d_materialise_unique(struct dentry \*, struct inode \*)" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/dcache.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_D_MATERIALISE_UNIQUE" ; \
	fi)
# KERNEL_HAS_D_MATERIALISE_UNIQUE Detection [END]

# KERNEL_HAS_PDE_DATA Detection [START]
#
# Find out whether the kernel has a PDE_DATA method.
#
# Note: This method was added in vanilla linux-3.10
#
KERNEL_HAS_PDE_DATA = $(shell \
	if grep "PDE_DATA(const struct inode \*)" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/proc_fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_PDE_DATA" ; \
	fi)
# KERNEL_HAS_PDE_DATA Detection [END]

# KERNEL_HAS_CURRENT_NSPROXY Detection [START]
#
# Find out whether the kernel has current->nsproxy
#
# Note: This was added in vanilla linux-2.6.19
#
KERNEL_HAS_CURRENT_NSPROXY = $(shell \
	if grep "struct nsproxy \*nsproxy;" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/sched.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_CURRENT_NSPROXY" ; \
	fi)
# KERNEL_HAS_CURRENT_NSPROXY Detection [END]


# KERNEL_HAS_GET_UNALIGNED_LE Detection [START]
#
# Find out whether the kernel has get_unaligned_le{16,32,64}
#
# Note: added to 2.6.26, but at least backported to RHEL6
#
KERNEL_HAS_GET_UNALIGNED_LE = $(shell \
	if grep "get_unaligned_le16" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/unaligned/access_ok.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_GET_UNALIGNED_LE" ; \
	fi)
# KERNEL_GET_UNALIGNED_LE Detection [END]


# KERNEL_HAS_I_UID_READ Detection [START]
#
# Find out whether the kernel has i_uid_read
#
# Note: added to 3.5
#
KERNEL_HAS_I_UID_READ = $(shell \
	if grep "i_uid_read" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_I_UID_READ" ; \
	fi)
# KERNEL_HAS_I_UID_READ Detection [END]


# KERNEL_HAS_ATOMIC_OPEN Detection [START]
#
# Find out whether the kernel has atomic_open
#
# Note: added to 3.5
#
KERNEL_HAS_ATOMIC_OPEN = $(shell \
	if grep "atomic_open" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_ATOMIC_OPEN" ; \
	fi)
# HAS_ATOMIC_OPEN  [END]


# KERNEL_HAS_UMODE_T Detection [START]
#
# Find out whether the kernel used umode_t
#
# Note: added to 3.3
# Note: Uses grep | grep, as backports might use fixed white spaces-
#
KERNEL_HAS_UMODE_T = $(shell \
	if grep -E "(\*mkdir).*umode_t" ${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_UMODE_T" ; \
	fi)
# KERNEL_HAS_UMODE_T  [END]


# KERNEL_HAS_2_PARAM_INIT_WORK
#
# Find out whether the kernel removed 1 parameter from INIT_WORK
#
# Note: INIT_WORK takes only two parameters since 2.6.20
# Note: Uses grep | grep, as backports might use fixed white spaces-
#
KERNEL_HAS_2_PARAM_INIT_WORK = $(shell \
	if grep -E "define INIT_WORK\(_work, _func\)" ${KSRCDIR_PRUNED_HEAD}/include/linux/workqueue.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_2_PARAM_INIT_WORK" ; \
	fi)
# KERNEL_HAS_2_PARAM_INIT_WORK  [END]


# KERNEL_HAS_NO_WRITE_CACHE_PAGES
#
# FIND out if the kernel has write_cache_pages
#
# Note: Added to 2.6.35, backported to RHEL6 kernels
# Note: Negative define to show write_cache_pages is the default,
#       needs to be tested with #ifndef instead of #ifdef
KERNEL_HAS_NO_WRITE_CACHE_PAGES = $(shell \
	if ! grep -E "write_cache_pages" ${KSRCDIR_PRUNED_HEAD}/include/linux/writeback.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_NO_WRITE_CACHE_PAGES" ; \
	fi)
# KERNEL_HAS_NO_WRITE_CACHE_PAGES [END]

# KERNEL_HAS_LIST_IS_LAST
#
# FIND out if the kernel has list_is_last
#
# Note: Added to 2.6.18
KERNEL_HAS_LIST_IS_LAST = $(shell \
	if grep -E "list_is_last" ${KSRCDIR_PRUNED_HEAD}/include/linux/list.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_LIST_IS_LAST" ; \
	fi)
# KERNEL_HAS_LIST_IS_LAST [END]

# KERNEL_HAS_CLEAR_PAGED_LOCKED
#
# FIND out if the kernel has __clear_page_locked
#
# Note: Added to 2.6.28
KERNEL_HAS_CLEAR_PAGE_LOCKED = $(shell \
	if grep -E "__clear_page_locked" ${KSRCDIR_PRUNED_HEAD}/include/linux/pagemap.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_CLEAR_PAGE_LOCKED" ; \
	fi)
# KERNEL_HAS_CLEAR_PAGED_LOCKED [END]

# KERNEL_HAS_DIRECT_IO_ITER
#
# Find out if the kernel has an iov_iter arg for the direct_IO method.
#
# Note: Added to vanilla 3.16, but backported e.g. to Oracle uek 3.8
KERNEL_HAS_DIRECT_IO_ITER = $(shell \
	if grep -E "(\*direct_IO).*struct iov_iter" ${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_DIRECT_IO_ITER" ; \
	fi)
# KERNEL_HAS_DIRECT_IO_ITER [END]

# KERNEL_HAS_SPECIAL_UEK_IOV_ITER
#
# Find out if the kernel has a special struct iov_iter (which does not contain an iov pointer) which
# only seems to exist in Oracle uek kernels.
KERNEL_HAS_SPECIAL_UEK_IOV_ITER = $(shell \
	if grep "struct iovec \*iov_iter_iovec" ${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_SPECIAL_UEK_IOV_ITER" ; \
	fi)
# KERNEL_HAS_SPECIAL_UEK_IOV_ITER [END]

# KERNEL_HAS_F_DENTRY [START]
#
# Find out if the kernel has f_dentry as part of struct dentry or a f_dentry define for
# f_path.dentry.
KERNEL_HAS_F_DENTRY = $(shell \
	if grep "f_dentry" ${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_F_DENTRY" ; \
	fi)
# KERNEL_HAS_F_DENTRY [END]

# KERNEL_HAS_FILE_INODE [START]
#
# Find out if the kernel has a file_inode() method.
KERNEL_HAS_FILE_INODE = $(shell \
	if grep -E " file_inode(.*)" ${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_FILE_INODE" ; \
	fi)
# KERNEL_HAS_FILE_INODE [END]

# KERNEL_HAS_GET_UNALIGNED_LE Detection [START]
#
# Find out whether the kernel has get_unaligned_le{16,32,64}
#
# Note: added to 2.6.26, but at least backported to RHEL6
#
KERNEL_HAS_GET_UNALIGNED_LE = $(shell \
	if grep "get_unaligned_le16" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/unaligned/access_ok.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_GET_UNALIGNED_LE" ; \
	fi)
# KERNEL_GET_UNALIGNED_LE Detection [END]

# KERNEL_HAS_STRNICMP Detection [START]
#
# Find out whether the kernel has a strnicmp function.
#
# Note: strnicmp was switched to strncasecmp in linux-4.0. strncasecmp existed
# before, but was wrong, so we only use strncasecmp if strnicmp doesn't exist.
#
KERNEL_HAS_STRNICMP = $(shell \
	if grep "strnicmp" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/string.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_STRNICMP" ; \
	fi)
# KERNEL_HAS_STRNICMP Detection [END]

# KERNEL_HAS_BDI_CAP_MAP_COPY Detection [START]
#
# Find out whether the kernel has BDI_CAP_MAP_COPY defined.
#
KERNEL_HAS_BDI_CAP_MAP_COPY = $(shell \
	if grep "define BDI_CAP_MAP_COPY" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/backing-dev.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_BDI_CAP_MAP_COPY" ; \
	fi)
# KERNEL_HAS_BDI_CAP_MAP_COPY Detection [END]

# KERNEL_HAS_POSIX_GET_ACL Detection [START]
#
# Find out whether the kernel has the get_acl inode operation
#
KERNEL_HAS_POSIX_GET_ACL = $(shell \
	if grep "struct posix_acl \* (\*get_acl)(struct inode \*, int);" \
		${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
	then echo "-DKERNEL_HAS_POSIX_GET_ACL" ; \
	fi)
# KERNEL_HAS_POSIX_GET_ACL Detection [END]

# KERNEL_HAS_CONST_XATTR_HANDLER Detection [START]
#
# Find out whether xattr_handler** s_xattr in super_block is const.
#
KERNEL_HAS_CONST_XATTR_HANDLER = $(shell \
   if grep "const struct xattr_handler \*\*s_xattr;" \
      ${KSRCDIR_PRUNED_HEAD}/include/linux/fs.h 1>/dev/null 2>&1 ; \
   then echo "-DKERNEL_HAS_CONST_XATTR_HANDLER" ; \
   fi)
# KERNEL_HAS_CONST_XATTR_HANDLER Detection [END]

# KERNEL_HAS_DENTRY_XATTR_HANDLER Detection [START]
#
# Find out whether xattr_handler functions need a dentry* (otherwise they need an inode*).
#
# Note: grepping for "(*set).struct..." instead of "(*set)(struct..." because make complains about
#       the missing ")" otherwise.
KERNEL_HAS_DENTRY_XATTR_HANDLER = $(shell \
   if grep "int (\*set).struct dentry \*dentry, const char \*name," \
      ${KSRCDIR_PRUNED_HEAD}/include/linux/xattr.h 1>/dev/null 2>&1 ; \
   then echo "-DKERNEL_HAS_DENTRY_XATTR_HANDLER" ; \
   fi)
# KERNEL_HAS_DENTRY_XATTR_HANDLER Detection [END]

# struct iov_iter was originally new to to 2.6.23, but some distro kernels have it too
# iov_iter_advance was added a little bit later
# Note: we need to check that in multiple files, because it is declared in uio.h in vanilla kernels, but at least RHEL backports put it in fs.h
$(call define_if_matches, KERNEL_HAS_IOV_ITER_TYPE, "struct iov_iter {", fs.h)
$(call define_if_matches, KERNEL_HAS_IOV_ITER_FUNCTIONS, "void iov_iter_advance", fs.h)
$(call define_if_matches, KERNEL_HAS_IOV_ITER_TYPE, "struct iov_iter {", uio.h)
$(call define_if_matches, KERNEL_HAS_IOV_ITER_FUNCTIONS, "void iov_iter_advance", uio.h)
 
# iov_iter_truncate was new to 3.16, but was backported in distro kernels
# Note: we need to check that in multiple files, because it is declared in uio.h in vanilla kernels, but at least RHEL backports put it in fs.h
$(call define_if_matches, KERNEL_HAS_IOV_ITER_TRUNCATE, "static inline void iov_iter_truncate(struct iov_iter \*i, size_t count)", fs.h) 
$(call define_if_matches, KERNEL_HAS_IOV_ITER_TRUNCATE, "static inline void iov_iter_truncate(struct iov_iter \*i, u64 count)", uio.h) 

# address_space.assoc_mapping went away in vanilla 3.8, but SLES11 backports that change
$(call define_if_matches, KERNEL_HAS_ADDRSPACE_ASSOC_MAPPING, -F "assoc_mapping", fs.h)

# current_umask() was added in 2.6.30
$(call define_if_matches, KERNEL_HAS_CURRENT_UMASK, -F "current_umask", fs.h)

# super_operations.show_options was changed to struct dentry* in 3.3
$(call define_if_matches, KERNEL_HAS_SHOW_OPTIONS_DENTRY, -F "int (*show_options)(struct seq_file *, struct dentry *);", fs.h)

# Find out whether ib_create_cq function has cq_attr argument
# This is tricky because the function declaration spans multiple lines.
# Note: Was introduced in vanilla 4.2
OFED_HAS_IB_CREATE_CQATTR += $(shell \
   grep -sA4 "struct ib_cq \*ib_create_cq(struct ib_device \*device," ${OFED_DETECTION_PATH}/rdma/ib_verbs.h 2>&1 \
      | grep -qs "const struct ib_cq_init_attr \*cq_attr);" \
      && echo "-DOFED_HAS_IB_CREATE_CQATTR")

# xattr handlers >=v4.4 also receive pointer to struct xattr_handler
KERNEL_FEATURE_DETECTION += $(shell \
   grep -s -F "int (*get)" ${KSRCDIR_PRUNED_HEAD}/include/linux/xattr.h \
      | grep -q -s -F "const struct xattr_handler *" \
      && echo "-DKERNEL_HAS_XATTR_HANDLER_PTR_ARG -DKERNEL_HAS_DENTRY_XATTR_HANDLER")

# 4.5 introduces name in xattr_handler, which can be used instead of prefix
KERNEL_FEATURE_DETECTION += $(shell \
   grep -sFA1 "struct xattr_handler {" ${KSRCDIR_PRUNED_HEAD}/include/linux/xattr.h \
      | grep -qsF "const char *name;" \
      && echo "-DKERNEL_HAS_XATTR_HANDLER_NAME")

# locks_lock_inode_wait is used for flock since 4.4 (before flock_lock_file_wait was used) 
$(call define_if_matches, KERNEL_HAS_LOCKS_LOCK_INODE_WAIT, -F "static inline int locks_lock_inode_wait(struct inode *inode, struct file_lock *fl)", fs.h)

# get_link() replaces follow_link() in 4.5
$(call define_if_matches, KERNEL_HAS_GET_LINK, -F "const char * (*get_link) (struct dentry *, struct inode *, struct delayed_call *);", fs.h)

# Combine results
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_DROP_NLINK)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_CLEAR_NLINK)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_WBC_NONRWRITEINDEXUPDATE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_TEST_LOCK_VOID_TWOARGS)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_ATTR_OPEN)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_IHOLD)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_TASK_IO_ACCOUNTING)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_FSYNC_RANGE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_INC_NLINK)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_S_D_OP)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_LOOKUP_CONTINUE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_D_MATERIALISE_UNIQUE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_PDE_DATA)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_CURRENT_NSPROXY)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_GET_UNALIGNED_LE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_I_UID_READ)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_ATOMIC_OPEN)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_UMODE_T)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_2_PARAM_INIT_WORK)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_NO_WRITE_CACHE_PAGES)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_LIST_IS_LAST)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_CLEAR_PAGE_LOCKED)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_DIRECT_IO_ITER)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_SPECIAL_UEK_IOV_ITER)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_F_DENTRY)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_FILE_INODE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_GET_UNALIGNED_LE)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_STRNICMP)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_BDI_CAP_MAP_COPY)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_POSIX_GET_ACL)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_CONST_XATTR_HANDLER)
KERNEL_FEATURE_DETECTION += $(KERNEL_HAS_DENTRY_XATTR_HANDLER)
KERNEL_FEATURE_DETECTION += $(OFED_HAS_IB_CREATE_CQATTR)