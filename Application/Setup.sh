#!/bin/bash

#Application path location of applicaiton

Dependencies=`pwd`/Dependencies

#source ${ToolDAQapp}/ToolDAQ/root/bin/thisroot.sh

export LD_LIBRARY_PATH=`pwd`/lib:${Dependencies}/zeromq-4.0.7/lib:${Dependencies}/boost_1_66_0/install/lib:${Dependencies}/ToolFrameworkCore/lib:${Dependencies}/ToolDAQFramework/lib:$LD_LIBRARY_PATH

export SEGFAULT_SIGNALS="all"
