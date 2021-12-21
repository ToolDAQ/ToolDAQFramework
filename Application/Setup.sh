#!/bin/bash

#Application path location of applicaiton

ToolDAQapp=`pwd`

#source ${ToolDAQapp}/ToolDAQ/root/bin/thisroot.sh

export LD_LIBRARY_PATH=`pwd`/lib:${ToolDAQapp}/lib:${ToolDAQapp}/Dependencies/zeromq-4.0.7/lib:${ToolDAQapp}/Dependencies/boost_1_66_0/install/lib:$LD_LIBRARY_PATH

export SEGFAULT_SIGNALS="all"
