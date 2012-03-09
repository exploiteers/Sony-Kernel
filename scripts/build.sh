#!/bin/sh
#
# kernel build script for NS internal build
#

###### Please define the parameters for each build   #############
export VER_PREFIX=BTV
export VER_EVENT=2
export VER_MAJOR=0
export VER_MINOR=0
export PVER=2_0_BTV_20110223
##################################################################

###### Please set the following environment parameters
# CANMOREDIR (intel stagingdir)

STAGINGDIR=$STAGINGDIR
PWD=$(pwd)
DATE=$(date +%Y%m%d)
export VER=$VER_EVENT.$VER_MAJOR.$VER_MINOR
export PNAME=kernel-$PVER
export PATH=$PATH:$STAGINGDIR/bin
export KERNELDIR=$PWD
export DISTDIR=$PWD/../../../build/$PNAME
export DISTTOP=$PWD/../../../build
export KERNEL_HEADER_DIR=$DISTDIR/kernel_headers
export WORKTMPDIR=$DISTDIR/tmp
export KERNEL_USB_MODULES=${PNAME}_usb_modules.tgz
export KERNEL_INTEL_MODULES=${PNAME}_intel_modules.tgz

export INTELCE_TOPDIR=${STAGINGDIR}/../../
export INTELCE_BUILD_DEST=$STAGINGDIR
export KHEADER=kernel2635-headers-$VER.tgz
export KBUILD=kernel2635-kbuild-$VER.tgz

export ARCH=x86
#export KBUILD_DEFCONFIG=btv2g_defconfig
export KBUILD_DEFCONFIG=btv2g_ht_defconfig
export CROSS_COMPILE=i686-cm-linux-

export OUTDIR=$KERNELDIR/../out/

build_kernel()
{
	#./setup-btv
	make ARCH=$ARCH KBUILD_DEFCONFIG=$KBUILD_DEFCONFIG defconfig
	echo $ARCH  > ./.arch_name
	echo $CROSS_COMPILE > ./.cross_compile
	make ARCH=$ARCH CORSS_COMPILE=$CROSS_COMPILE
	${CROSS_COMPILE}objcopy -O binary vmlinux vmlinux.bin
	make ARCH=$ARCH CORSS_COMPILE=$CROSS_COMPILE modules
	./scripts/make_kernel_headers.sh
	#make kernel_headers
	#make modules
}

install_kernel_headers()
{

	CROSS_COMPILE=i386-sony-linux 
	PATH=/usr/bin:/bin:$PATH
	mkdir -p $DISTDIR/kernel_headers
	sudo ./scripts/install_kernel_headers.sh --top=$KERNEL_HEADER_DIR kernel_headers.tar.gz

	tar -c -z -f $KERNEL_HEADER_DIR/$KHEADER -C $DISTDIR/kernel_headers/x86/target/devel .
	tar -c -z -f $KERNEL_HEADER_DIR/$KBUILD -C $DISTDIR/kernel_headers/x86/kbuild .
}

prepare_kbuild()
{
    ./scripts/make_kernel_headers.sh
    mkdir -p $OUTDIR
    sudo ./scripts/install_kernel_headers.sh --top=$OUTDIR kernel_headers.tar.gz
    #tar -c -z -f $OUTDIR/$KBUILD -C $OUTDIR/kernel_headers/x86/kbuild .
}

install_kernel()
{
	mkdir -p $DISTDIR/bin
	mkdir -p $DISTDIR/modules
	if [ ! -d $DISTTOP/$VER ] ; then
	cd $DISTTOP
	ln -s $PNAME $VER
	fi
	#cd $PWD
	cd $KERNELDIR
	mkdir -p $WORKTMPDIR
	mkdir -p $WORKTMPDIR/lib/modules
	
	install -m 644 System.map $DISTDIR/bin
	install -m 644 .config $DISTDIR/bin/config
	install -m 644 vmlinux $DISTDIR/bin
	install -m 644 arch/$ARCH/boot/compressed/vmlinux.bin $DISTDIR/bin 

	md5sum $DISTDIR/bin/vmlinux.bin >$DISTDIR/bin/MD5SUM

	make modules_install INSTALL_MOD_PATH=$WORKTMPDIR
	usb_modules=`find $WORKTMPDIR/lib/modules -name *.ko ! -name scsi_wait_scan.ko` 
	install -m 644 $usb_modules $DISTDIR/modules/
	rm -rf $WORKTMPDIR
}

make_package()
{
	mkdir -p $DISTDIR/package
	mkdir -p $WORKTMPDIR/lib/modules

	if [ -d $DISTDIR/bzImage ]; then
		install -m 644 $DISTDIR/bzImage $WORKTMPDIR
	fi
	install -m 644 $DISTDIR/modules/*.ko $WORKTMPDIR/lib/modules/
	tar -c -z -f $DISTDIR/package/$KERNEL_USB_MODULES -C $WORKTMPDIR .
	rm -rf $WORKTMPDIR
}

build_intel_modules()
{
	if [ ! -d $INTELCE_TOPDIR ]; then
		echo "Cannot find Intel SDK"
		echo "Please install and set $CANMOREDIR env value"
		return
	fi

	make -C $INTELCE_TOPDIR defconfig
	make -C $INTELCE_TOPDIR kernel-unpackage

	linux32 make -C $INTELCE_TOPDIR toolchain
	rm -rf $INTELCE_BUILD_DEST/usr/include/asm
	rm -rf $INTELCE_BUILD_DEST/usr/include/asm-generic
	rm -rf $INTELCE_BUILD_DEST/usr/include/asm-i386
	rm -rf $INTELCE_BUILD_DEST/usr/include/acpi
	rm -rf $INTELCE_BUILD_DEST/usr/include/linux
	tar -x -z -f $KERNEL_HEADER_DIR/$KHEADER -C $INTELCE_BUILD_DEST
	linux32 make -C $INTELCE_TOPDIR kernel
	rm -rf $INTELCE_BUILD_DEST/kernel/linux-2.6.23/*
	tar -x -z -f $KERNEL_HEADER_DIR/$KBUILD -C $INTELCE_BUILD_DEST/kernel/linux-2.6.23
	sudo linux32 make -C $INTELCE_TOPDIR all

}

install_intel_modules()
{
	mkdir -p $WORKTMPDIR/lib/modules
	mkdir -p $WORKTMPDIR/lib/modules/sound/acore/seq
	mkdir -p $WORKTMPDIR/lib/modules/sound/usb

	mkdir -p $DISTDIR/intel_modules

	for m in ${INTEL_MODULES[@]} ; \
	do \
		install -m644 $INTELCE_TOPDIR/project_build_i686/IntelCE/root/lib/modules/$m $WORKTMPDIR/lib/modules/$m ; \
	done

	tar -c -z -f $DISTDIR/package/$KERNEL_INTEL_MODULES -C $WORKTMPDIR .
	cp -a $WORKTMPDIR/lib/modules/* $DISTDIR/intel_modules/.
	rm -rf $WORKTMPDIR
}

#### main
#build_kernel
#install_kernel
#install_kernel_headers
#make_package

# build_intel_modules
# install_intel_modules
prepare_kbuild
