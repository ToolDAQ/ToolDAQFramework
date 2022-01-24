### Created by Dr. Benjamin Richards

### Download base image from cern repo on docker hub
FROM toolframework/core

### Run the following commands as super user (root):
USER root


RUN git clone https://github.com/ToolDAQ/zeromq-4.0.7.git \
    && cd zeromq-4.0.7 \
    && ./configure --prefix=`pwd` \
    && make -j `nproc --all` \
    && cd ../ \
    && git clone https://github.com/ToolDAQ/boost_1_66_0.git \
    && cd boost_1_66_0 \
    && rm -rf INSTALL \
    && mkdir install \
    && ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null \
    && ./b2 install iostreams -j `nproc --all` \
    && cd ../ \
    && git clone https://github.com/ToolDAQ/ToolDAQFramework.git \
    && cd ToolDAQFramework \
    && make clean \
    && make -j `nproc --all` 


### Open terminal
CMD ["/bin/bash"]