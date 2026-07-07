#!/bin/bash
#tool script used by project maintainer to test debian releases
# WIP - needs maintainer to create .dsc and debian directory - current takes from CPack

VERSION=$(cat ../CMakeLists.txt |grep set\(HARDINFO2_VERSION|cut -d '"' -f 2)
ARCH=$(uname -m)
if [ $ARCH=="x86_64" ]; then ARCH=amd64; fi

cd ..
rm -rf build
sudo apt -y remove hardinfo
sudo apt -y remove hardinfo2

mkdir build
cd build
cmake ..
make package_source
#rename cpack files
mv hardinfo2_$VERSION*.deb hardinfo2-$VERSION.src.deb
rm hardinfo2_$VERSION*.tar.gz

#extract CPack source package
mkdir cpacksrc
dpkg-deb -R hardinfo2-$VERSION.src.deb cpacksrc

#create source package (NOTE: We use github tags as release-$VERSION)
# create clean orig tarball, excluding build, .git, .github
cd ..
tar -czf build/hardinfo2_$VERSION.orig.tar.gz --exclude='build' --exclude='.git' --exclude='.github' --transform "s,^,hardinfo2-$VERSION/," .
cd build

#extract source
tar -xzf hardinfo2_$VERSION.orig.tar.gz
cd hardinfo2-$VERSION
debmake
#fixup source from cpack - FIXME
cd debian
grep Maintainer ../../cpacksrc/DEBIAN/control >control.fixed
grep -v Homepage control |grep -v Description|grep -v auto-gen|grep -v Section|grep -v debmake |grep -v Maintainer >>control.fixed
echo "Homepage: https://hardinfo2.org">>control.fixed
echo "Description: Hardinfo2 - System Information and Benchmark" >>control.fixed
grep Recommends ../../cpacksrc/DEBIAN/control >>control.fixed
grep Section ../../cpacksrc/DEBIAN/control >>control.fixed
rm -f control
mv control.fixed control
cd ..

#create debian tar.gz
tar -czf ../hardinfo2_$VERSION.debian.tar.gz debian
cd ..

#create dsc - match the exact format that passed dpkg-source
cat > ./hardinfo2-$VERSION.dsc <<DSCHEADER
Format: 3.0 (quilt)
Source: hardinfo2
Binary: hardinfo2
Architecture: any
Version: $VERSION
Maintainer: $(grep Maintainer ./cpacksrc/DEBIAN/control | cut -d' ' -f2-)
Homepage: https://hardinfo2.org
Description: System Information and Benchmark for Linux Systems
 Hardinfo offers System Information and Benchmark for Linux Systems.
 It is able to obtain information from both hardware and basic software.
 It can benchmark your system and compare to other machines online.
Vcs-Browser: https://salsa.debian.org/hwspeedy/hardinfo2
Vcs-Git: https://salsa.debian.org/hwspeedy/hardinfo2.git
Build-Depends: cmake, debhelper (>= 11)
Package-List:
 hardinfo2 deb x11 optional arch=any
Checksums-Sha1:
DSCHEADER

# Append Checksums-Sha1 lines (each starting with a space)
for f in hardinfo2_$VERSION.debian.tar.gz hardinfo2_$VERSION.orig.tar.gz; do
    sha1=$(sha1sum "$f" | cut -d' ' -f1)
    size=$(stat -c%s "$f")
    echo " $sha1 $size $f" >>./hardinfo2-$VERSION.dsc
done

echo "Checksums-Sha256:" >>./hardinfo2-$VERSION.dsc
for f in hardinfo2_$VERSION.debian.tar.gz hardinfo2_$VERSION.orig.tar.gz; do
    sha256=$(sha256sum "$f" | cut -d' ' -f1)
    size=$(stat -c%s "$f")
    echo " $sha256 $size $f" >>./hardinfo2-$VERSION.dsc
done

echo "Files:" >>./hardinfo2-$VERSION.dsc
for f in hardinfo2_$VERSION.debian.tar.gz hardinfo2_$VERSION.orig.tar.gz; do
    md5=$(md5sum "$f" | cut -d' ' -f1)
    size=$(stat -c%s "$f")
    echo " $md5 $size $f" >>./hardinfo2-$VERSION.dsc
done

echo "Debian Source Package Files ready in build:"
ls -l hardinfo2-$VERSION*.tar.gz
ls -l hardinfo2-$VERSION.dsc

#build from source
sudo apt install debhelper
cd hardinfo2-$VERSION
debuild -b -uc -us

#test package
ls ../hardinfo2_*.deb
sudo apt -y install ../hardinfo2_$VERSION-1_$ARCH.deb
apt info hardinfo2
