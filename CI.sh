#!/bin/bash

mkdir ToolDAQ
cd ToolDAQ

git clone https://github.com/ToolDAQ/ToolDAQFramework.git
git clone https://github.com/ToolDAQ/zeromq-4.0.7.git
wget http://downloads.sourceforge.net/project/boost/boost/1.60.0/boost_1_60_0.tar.gz
tar zxf boost_1_60_0.tar.gz

cd zeromq-4.0.7

./configure --prefix=`pwd`
make
make install

export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH

cd ../boost_1_60_0

mkdir install

./bootstrap.sh --prefix=`pwd`/install/ > /dev/null 2>/dev/null
./b2 install

export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH

cd ../ToolDAQFramework

make clean
make

export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH

cd ..

wget https://root.cern.ch/download/root_v5.34.34.source.tar.gz
tar zxf root_v5.34.34.source.tar.gz
cd root

./configure --prefix=`pwd` --enable-rpath > /dev/null 2>/dev/null
make
make install

source ./bin/thisroot.sh

cd ../../

make clean
make

./main configfiles/ToolChainConfig