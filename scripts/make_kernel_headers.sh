#!/bin/sh
#
#  Script to make kernel header
#
#  Copyright 2006-2009 Sony Corporation.
#
set -e # stop on error

DEST=kernel_headers.tar.gz
HEADER=kernel_header.tar
KBUILD=kbuild_tree.tar

srctree=${srctree:=.}
objtree=${objtree:=.}

FIXUP=${objtree}/.fixup_kernel_headers
SYMVERS=${objtree}/Module.symvers
VERFILE=${objtree}/include/linux/version.h
WORK=${objtree}/.work

#
# Files and directories required other than KConfig/Makefiles.
#
COPY_DIRS="scripts/"

# COPY_FILES should be synced with install_kernel_headers.sh.
#COPY_FILES=".config .arch_name .cross_compile .target_name"
COPY_FILES=".config .arch_name .cross_compile"

fatal () {
    echo "FATAL: $*" > /dev/tty
    rm -rf ${WORK}
    exit 1
}

usage () {
    echo "usage: $0 [ output_file_name.tar.gz ]" > /dev/tty
    exit 0
}

if [ "$#" -ge 2 ]; then
    usage
fi
if [ $# == 1 ]; then
    DEST=$1
fi

test -f ${VERFILE} || fatal "kernel is not configured"

for f in ${COPY_FILES}; do
    test -f ${objtree}/${f} || fatal "${f} is not found"
done
for d in ${COPY_DIRS} ; do
    test -d ${srctree}/${d} || fatal "${d} is not found"
done

ARCH=`cat ${objtree}/.arch_name`
echo "Creating kernel headers (${DEST}) for ${ARCH}..."

#
# make kernel headers
#
rm -rf ${WORK}
mkdir -p ${WORK}

# copy all header files
for d in ${srctree} ${objtree} ; do
    tar cf - -C ${d}/include . | (cd ${WORK} ; tar xf -)
	tar cf - -C ${d}/arch/x86/include ./asm | (cd ${WORK} ; tar xf -)
done

# fixup header files
if [ -e ${FIXUP} ]; then
    sh ${FIXUP} ${WORK}
fi

# remove unnecessary directories/files
find ${WORK} -type d \( -name "CVS" -o -name ".svn" \) -print | xargs rm -rf
find ${WORK} -type f -name "*~" -print | xargs rm -rf
find ${WORK} -type f -name "ashmem.h" -o -name "android_alarm.h" | xargs rm -rf

# change owner/permission
find ${WORK} -type f -print | xargs chmod 644
find ${WORK} -type d -print | xargs chmod 755

# create tarball
tar cf ${HEADER} -C ${WORK} .
rm -r ${WORK}
test -f ${HEADER}
if [ $? -ne 0 ]; then
    echo
    fatal "Cannot create kernel header"
fi

#
# make kbuild tree
#
mkdir -p ${WORK}
(cd ${srctree};
find . -type f \( -name "Makefile*" -o -name "Kconfig*" \) \
             ! \( -path "*.work*" -o -path "*.svn*" -o -path "*.pc*" \) | \
             cpio -o --format=newc --quiet | (cd ${WORK}; cpio -iumd --quiet))
for f in ${COPY_FILES} ; do
    cp ${objtree}/${f} ${WORK}
done
for d in ${COPY_DIRS} ; do
    cp -R  ${srctree}/${d} ${WORK}
    cp -fR ${objtree}/${d} ${WORK}
done
if [ -e ${SYMVERS} ]; then
    cp ${SYMVERS} ${WORK}
fi

# remove unnecessary directory
rm -rf ${WORK}/Documentation
rm -rf ${WORK}/include
rm -rf ${WORK}/.pc
rm -rf ${WORK}/patches

# remove unnecessary directories/files
find ${WORK} -type d \( -name "CVS" -o -name ".svn" \) -print | xargs rm -rf
find ${WORK} -type f -name "*~" -print | xargs rm -rf

# change permission
find ${WORK} -type d -print | xargs chmod 755

# create tarball
tar cf ${KBUILD} -C ${WORK} .
rm -r ${WORK}
test -f ${KBUILD}
if [ $? -ne 0 ]; then
    echo
    fatal "Cannot create kbuild tree"
fi

#
# make archive file
#
tar cfz ${DEST} ${HEADER} ${KBUILD}
rm ${HEADER} ${KBUILD}

echo "done"
ls -l ${DEST}

exit 0
