#!/bin/bash
echo ""
echo "This script builds+installs sysbench 1.0.20 for epel10 (Redhat,Oracle,Alme,Rocky...)"
echo ""
ARCH=$( uname -m )
echo $ARCH;
echo "needs additional packages to hardinfo2: libtool automake"
sudo yum install libtool automake

#Latest release 1.0.20
git clone https://github.com/akopytov/sysbench --branch 1.0

#update luajit for riscv
#cd sysbench/third_party/luajit
#rm -rf luajit
#git clone https://github.com/plctlab/LuaJIT/ luajit

#update ck for riscv
#cd ../concurrency_kit
#rm -rf ck
#git clone https://github.com/concurrencykit/ck ck
#cd ../../

cd sysbench

#build
./autogen.sh
./configure --without-mysql
make -j

#only local install sysbench
sudo cp src/sysbench /usr/local/bin/

#cleanup
cd ..
rm -rf sysbench

echo "Done - Sysbench is installed in /usr/local/bin"
