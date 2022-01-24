### Created by Dr. Benjamin Richards

### Download base image from cern repo on docker hub
FROM tooldaq/base

### Run the following commands as super user (root):
USER root


RUN cd /opt \
    && git clone https://github.com/ToolDAQ/ToolDAQFramework.git \
    && cd ToolDAQFramework \
    && make clean \
    && make -j `nproc --all` 


### Open terminal
CMD ["/bin/bash"]