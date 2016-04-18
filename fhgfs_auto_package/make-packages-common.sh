#!/bin/bash

set -e

DATE=`date +%F|tr - .`.`date +%T| tr -d :`
dir=`dirname $0`

MAJOR_VER=`${dir}/../beegfs-version --print_major_version`
RELEASE_STR=`${dir}/../beegfs-version --print_release_version`

packages="beegfs_helperd beegfs_meta beegfs_mgmtd beegfs_storage beegfs_utils          \
	beegfs_common_package beegfs_client_devel beeond beeond_thirdparty_gpl"
 
client_packages="beegfs_client_module"
admon_package="beegfs_admon"
opentk_package="beegfs_opentk_lib" # deletes the libopentk.so, so needs to be last

export CONCURRENCY=${MAKE_CONCURRENCY:-4}
PACKAGEDIR="/tmp/beegfs_packages-${DATE}/"


# print usage information
print_usage()
{
	echo
	echo "USAGE:"
	echo "  $ `basename $0` [options]"
	echo
	echo "OPTIONS:"
        echo "  -v S   Major version string (e.g. \"2015.03\")."
	echo "         Default is based on current date."
        echo "  -s S   Minor version string (e.g. \"r1\")."
        echo "         Default is based on current date."
        echo "  -j N   Number of parallel processes for \"make\"."
        echo "  -p S   Package destination directory."
	echo "         Default is a subdir of \"/tmp\"."
	echo "  -c     Run \"make clean\" only."
	echo "  -d     Dry-run, only print export variables."
	echo "  -D     Disable beegfs-admon package build."
	echo "  -C     Build client packages only."
	echo "  -x     Build with BEEGFS_DEBUG."
	echo
	echo "EXAMPLE:"
	echo "  $ `basename $0` -j 4 -p /tmp/my_beegfs_packages"
	echo
}

# print given command, execute it and terminate on error return code
run_cmd()
{
	cmd="$@"
	echo "$cmd"
	eval $cmd
	res=$?
	if [ $res -ne 0 ]; then
		echo " failed: $res"
		echo "Aborting"
		exit 1
	fi
}

# parse command line arguments

DRY_RUN=0
CLEAN_ONLY=0
CLIENT_ONLY=0

while getopts "hcds:v:DCxj:p:" opt; do
	case $opt in
	h)
		print_usage
		exit 1
		;;
	c)
		CLEAN_ONLY=1
		;;
	d)
		DRY_RUN=1
		;;
	s)	
		RELEASE_STR="$OPTARG"
                ;;
	v)
		MAJOR_VER="$OPTARG"
		;;
	D)
		admon_package=""
		echo "Admon package disabled."
		;;
	C)
		CLIENT_ONLY=1
		;;
	x)
		export BEEGFS_DEBUG=1
		;;
	j)
		export CONCURRENCY="$OPTARG"
		export MAKE_CONCURRENCY="$OPTARG"
		;;
	p)
		PACKAGEDIR="$OPTARG"
		;;
        :)
                echo "Option -$OPTARG requires an argument." >&2
                print_usage
                exit 1
                ;;
        *)
                echo "Invalid option: -$OPTARG" >&2
                print_usage
                exit 1
                ;;
        esac
done


if [ $CLIENT_ONLY -eq 0 ]; then
	echo "Building all packages."
	packages="$packages $admon_package $opentk_package $client_packages"
else
	echo "Building client packages only."
	packages=$client_packages
fi

BEEGFS_VERSION="${MAJOR_VER}-${RELEASE_STR}"

echo
run_cmd "export VER=${MAJOR_VER}"
run_cmd "export RELEASE_STR=$RELEASE_STR"
run_cmd "export BEEGFS_VERSION=$BEEGFS_VERSION"
echo

opentk="beegfs_opentk_lib"
common="beegfs_common"
thirdparty="beegfs_thirdparty"

pushd `dirname $0`/../
ROOT=`pwd`

if [ $DRY_RUN -eq 1 ]; then
	exit 0
fi


# build dependency libs
make_dep_lib()
{
	lib="$1"
	parallel_override="$2"

	if [ -n "$parallel_override" ]; then
		make_concurrency=$parallel_override
	else
		make_concurrency=$CONCURRENCY
	fi

	echo "CONCURRENCY: using 'make -j $make_concurrency'"

	echo ${lib}
	pwd
	run_cmd "make -C ${lib}/${EXTRA_DIR}/build clean --silent >/dev/null"
	run_cmd "make -C ${lib}/${EXTRA_DIR}/build -j $make_concurrency --silent >/dev/null"

}

# clean packages up here first, do not do it below, as we need
# common and opentk
for package in $packages $thirdparty $opentk $common; do
	echo $package clean
	make -C ${package}/${EXTRA_DIR}/build clean --silent
done
	
if [ ${CLEAN_ONLY} -eq 1 ]; then
	exit 0
fi


# build common, opentk and thirdparty, as others depend on them
if [ $CLIENT_ONLY -eq 0 ]; then
	make_dep_lib $thirdparty
	make_dep_lib $common
	make_dep_lib $opentk 
fi


mkdir -p $PACKAGEDIR
export DEBIAN_ARCHIVE_DIR=$PACKAGEDIR

