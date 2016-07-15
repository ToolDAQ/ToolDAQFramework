#include "../Unity.cpp"

Tool* Factory(std::string tool){
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;

if (tool=="BoardReader") ret=new BoardReader;
if (tool=="Filter") ret=new Filter;
if (tool=="RootWriter") ret=new RootWriter;
  if (tool=="BinaryWriter") ret=new BinaryWriter;
return ret;
}

