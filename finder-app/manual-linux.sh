#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # Note: Steps taken from lecture "Week 2: Building the Linux Kernel" 
    echo "1) Clean the build"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper
    echo "2) Configure a virtualized Arm board"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    echo "3) Build a kernel image for booting with QEMU."
    # Note: Needed to install "flex", "bison" and "libssl-dev" using apt to complete this phase
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all
    echo "4) Build kernel modules"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules
    echo "5) Build the devicetree binaries"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
    # TODO: End

fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/$ARCH/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
# Note: Steps taken from lecture "Week 2: Linux Root Filesystem (6:25) and MELP (p.196) "
echo "6) Create necessary base directories"
mkdir rootfs
cd "${OUTDIR}/rootfs"
mkdir bin dev etc lib proc sys sbin tmp usr var home
mkdir usr/bin usr/sbin
mkdir usr/lib
mkdir -p var/log
# TODO: End

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    # TODO: Configure busybox
    # Note: Steps taken from lecture "Week 2: Linux Root Filesystem (9:35) and MELP (p.206)"
    echo "7) Configure BusyBox"
    make distclean
    make defconfig
    # TODO: End

else
    cd busybox
fi

# TODO: Make and install busybox
# Note: Steps taken from lecture "Week 2: Linux Root Filesystem (10:45) and MELP (p.206)"
echo "8) Make and install BusyBox"
# ** Uncommented to save time. Uncomment if you wish to recompile.
sudo chmod u+s ${OUTDIR}/busybox 
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX=${OUTDIR}/rootfs install
# TODO: End

echo "Library dependencies"
cd "${OUTDIR}/rootfs"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "9) Add library dependencies"
SYSROOT=`${CROSS_COMPILE}gcc -print-sysroot`
sudo cp -a "${SYSROOT}"/lib/* lib/
sudo cp -a "${SYSROOT}"/lib64 .
# TODO: End

# TODO: Make device nodes
# Note: Steps taken from lecture "Week 2: Linux Root Filesystem (11:40)"
# There are two devices you need to boot a device initially: 1) Null device 2) Console device
echo "10) Make device nodes"
sudo mknod -m 666 dev/null c 1 3 # Null device
sudo mknod -m 666 dev/null c 5 1 # Console device.
# TODO: End

# TODO: Clean and build the writer utility
echo "11) Clean and build the writer utility"
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE
# TODO: End


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "12) Copy the finder scripts to the rootfs /home"
cd $FINDER_APP_DIR
cp -a writer autorun-qemu.sh finder-test.sh finder.sh "${OUTDIR}/rootfs/home"
mkdir "${OUTDIR}/rootfs/home/conf"
cp -a conf/username.txt "${OUTDIR}/rootfs/home/conf"
# TODO: End

# TODO: Chown the root directory
echo "13) Chown the root directory"
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *
# TODO: End

# TODO: Create initramfs.cpio.gz
echo "14) Create initramfs.cpio.gz file"
cd "$OUTDIR/rootfs"
find * . | cpio -o --format=newc > ../initramfs.cpio
gzip -c ../initramfs.cpio > ../initramfs.cpio.gz
# TODO: End

echo "Process Complete"