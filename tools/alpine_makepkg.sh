#!/bin/sh
#"CPACK" - called in build dir
echo "Building Alpine Package"
cd ..
cp -f ./tools/APKBUILD .
#cp -f ./tools/hardinfo2.install .

#build
if [ $(id -u) -ne 0 ]; then
    abuild -r
else
    abuild -r -F
#    chown -R nobody ../hardinfo2
#    sudo -u nobody abuild -r
#    chown -R root ../hardinfo2
fi

#cleanup
rm -f APKBUILD
#rm -f hardinfo2.install

#rename and move result to build dir
mv ~/packages/$USER/aarch64/*.apk ./build/
cd build
DISTRO=$(cat /etc/os-release |grep ^PRETTY_NAME=|awk '{sub(" ","-");sub("PRETTY_NAME=","");sub("\"","");sub("\"","")}1')
rename r1 hardinfo2-${DISTRO} *.apk
