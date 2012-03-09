#!/bin/sh
#
#  Script to install kernel header
#
#  Copyright 2006,2007 Sony Corporation.
#
set -e # stop on error

WORK=`pwd`/.work_install_kernel_headers
HEADER=${WORK}/kernel_header.tar
KBUILD=${WORK}/kbuild_tree.tar

# COPY_FILES should be synced with make_kernel_headers.sh.
#COPY_FILES=".config .arch_name .cross_compile .target_name"
COPY_FILES=".config .arch_name .cross_compile"


usage() {
    echo "$0 [--top=DIR] <kernel_headers_archive>" > /dev/tty
    exit 1
}

fatal () {
    echo "FATAL: $*" > /dev/tty
    rm -rf ${WORK}
    exit 1
}

while [ $# != 0 ]; do
    case $1 in
      --top | -top)
	shift
	TOP=$1
	shift
	;;
      --top=* | -top=*)
	TOP=`expr "x$1" : 'x[^=]*=\(.*\)'`
	shift
	;;
      --help)
        usage
        ;;
      *)
        break
    esac
done

test "$#" -ne 1 && usage
FILE=$1
test -f ${FILE} || usage
test "_`id -u`" == "_0" || fatal "Must be superuser"

rm -rf ${WORK}
mkdir -p ${WORK}

#
# extract archive files
#
tar xfz ${FILE} -C ${WORK}
if [[ ! -f ${HEADER} || ! -f ${KBUILD} ]]; then
    rm -r ${WORK}
    fatal "Invalid file"
fi

for f in ${COPY_FILES} ; do
    tar xf ${KBUILD} -C ${WORK} ./${f}
done

#
# create install path
#
ARCH=`cat ${WORK}/.arch_name`
CROSS_COMPILE=`cat ${WORK}/.cross_compile`

if [ -z "$TOP" ]; then
    # no --top.  use old style dir
    case ${ARCH} in
	arm)
	    if [ "${CROSS_COMPILE}" == "arm-sony-linux-gnueabi-dev-" ]; then
		INSTDIR=/usr/local/arm-sony-linux-gnueabi
	    else
		INSTDIR=/usr/local/arm-sony-linux
	    fi
	    ;;
	i386 | x86)
	    INSTDIR=/usr/local/i686-sony-linux
	    ;;
	mips)
	    INSTDIR=/usr/local/mips-sony-linux
	    ;;
	ppc)
	    INSTDIR=/usr/local/powerpc-sony-linux
	    ;;
	powerpc)
	    INSTDIR=/usr/local/powerpc-sony-linux
	    ;;
	*)
	    echo
	    fatal "${ARCH} is not supported"
	    ;;
    esac
else
    # --top specified.  use $TOP/<ARCH> where ARCH is taken from tripplet
    #sub=`expr "x$CROSS_COMPILE" : 'x\(.*\)-sony-linux.*'`
    sub=${ARCH}
    INSTDIR="$TOP/$sub"
fi

#
# subdirs of include
#
case ${ARCH} in
    arm)
	ARCH_DIRS="asm-${ARCH}"
        ;;
    i386 | x86)
	ARCH_DIRS="asm-${ARCH} acpi"
        ;;
    mips)
	ARCH_DIRS="asm-${ARCH}"
        ;;
    ppc)
        ARCH_DIRS="asm-ppc asm-powerpc platforms syslib"
        ;;
    powerpc)
	ARCH_DIRS="asm-${ARCH} platforms sysdev"
        ;;
    *)
        echo
        fatal "${ARCH} is not supported"
        ;;
esac
#DIRS="asm asm-generic linux ${ARCH_DIRS}"
DIRS="asm asm-generic linux generated"

RELINCDIR=${INSTDIR}/target/usr/include
DEVINCDIR=${INSTDIR}/target/devel/usr/include
KBUILDDIR=${INSTDIR}/kbuild

echo -n "Installing kernel headers for ${ARCH} to ${INSTDIR}..."

#
# install headers for kbuild
#
rm -rf ${KBUILDDIR}
mkdir -p ${KBUILDDIR}/include
tar xf ${KBUILD} -C ${KBUILDDIR}
tar xf ${HEADER} -C ${KBUILDDIR}/include

#
# install headers for user program
#
mkdir -p ${DEVINCDIR}
mkdir -p ${RELINCDIR}

for dir in ${DIRS}; do
     rm -rf ${DEVINCDIR}/${dir}
     rm -rf ${RELINCDIR}/${dir}
     cp -a ${KBUILDDIR}/include/${dir} ${DEVINCDIR}/${dir}
     cp -a ${KBUILDDIR}/include/${dir} ${RELINCDIR}/${dir}
done

# chmod files
chown -R root:root ${KBUILDDIR}
chown -R root:root ${DEVINCDIR}
chown -R root:root ${RELINCDIR}

# make sure the Makefile and version.h have a matching timestamp
touch -r ${KBUILDDIR}/Makefile ${DEVINCDIR}/linux/version.h
touch -r ${KBUILDDIR}/.config ${DEVINCDIR}/linux/autoconf.h

rm -r ${WORK}

echo "done"

exit 0
