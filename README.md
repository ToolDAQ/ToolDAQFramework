![ToolDAQ](https://user-images.githubusercontent.com/14093889/150870532-8f276868-0cf9-43ad-8dfc-2c9b6a821c32.png)

ToolDAQ is an open source general modular DAQ FrameWork, with built in service discovery and serialisation. It is based on the ToolFramework https://github.com/ToolFramework/ToolFramework

# *PLEASE NOTE: This is the core framework only!!! do not clone this repo for building your own application.
To create your own ToolDAQ appliciaion please clone/fork the ToolApplication repository https://github.com/ToolFramework/ToolApplication which has a script ```GetToolDAQ.sh``` to pull this repository down as a dependancy and set up everything for you.

****************************
#Concept
****************************

The main executable creates a ToolChain which is an object that holds Tools. Tools are added to the ToolChain and then the ToolChain can be told to Initialise Execute and Finalise each tool in the chain.

The ToolChain also holds a uesr defined DataModel which each tool has access too and can read ,update and modify. This is the method by which data is passed between Tools.

User Tools can be generated for use in the tool chain by incuding a Tool header. This can be done manually or by use of the newTool.sh script.

For more information consult the ToolDAQ manual:

https://docs.google.com/document/d/18rgYMOAGt3XiW9i0qN9kfUK1ssbQgLV1gQgG3hyVUoA

or the Doxygen docs

docs http://tooldaq.github.io/ToolDAQFramework


Copyright (c) 2016 Benjamin Richards (benjamin.richards@warwick.ac.uk)
