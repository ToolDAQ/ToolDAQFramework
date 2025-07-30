Dependencies=Dependencies
ToolFrameworkCore=$(Dependencies)/ToolFrameworkCore
ToolDAQFramework=$(Dependencies)/ToolDAQFramework
SOURCEDIR=`pwd`

CXXFLAGS=  -fPIC -std=c++11 -Wno-comment # -Wpedantic -Wall -Wno-unused -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept  -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef #-Werror -Wold-style-cast 


ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS+= -O0 -g -lSegFault -rdynamic -DDEBUG
else
CXXFLAGS+= -O3
endif

DataModelInclude =
DataModelLib =

MyToolsInclude =
MyToolsLib =

ZMQLib= -L $(Dependencies)/zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I $(Dependencies)/zeromq-4.0.7/include/ 

BoostLib= -L $(Dependencies)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I $(Dependencies)/boost_1_66_0/install/include

Includes=-I $(ToolFrameworkCore)/include/ -I $(ToolDAQFramework)/include/ -I $(SOURCEDIR)/include/ $(ZMQInclude) $(BoostInclude)
ToolLibraries = $(patsubst %, lib/%, $(filter lib%, $(subst /, , $(wildcard UserTools/*/*.so))))
LIBRARIES=lib/libDataModel.so lib/libMyTools.so $(ToolLibraries)
DataModelHEADERS:=$(patsubst %.h, include/%.h, $(filter %.h, $(subst /, ,$(wildcard DataModel/*.h))))
MyToolHEADERS:=$(patsubst %.h, include/%.h, $(filter %.h, $(subst /, ,$(wildcard UserTools/*/*.h) $(wildcard UserTools/*.h))))
ToolLibs = $(patsubst %.so, %, $(patsubst lib%, -l%,$(filter lib%, $(subst /, , $(wildcard UserTools/*/*.so)))))
AlreadyCompiled = $(wildcard UserTools/$(filter-out %.so UserTools , $(subst /, ,$(wildcard UserTools/*/*.so)))/*.cpp)
SOURCEFILES:=$(patsubst %.cpp, %.o,  $(filter-out $(AlreadyCompiled), $(wildcard */*.cpp) $(wildcard */*/*.cpp)))
Libs=-L $(SOURCEDIR)/lib/ -lDataModel -L $(ToolDAQFramework)/lib/ -lToolDAQChain -lDAQDataModelBase  -lDAQLogging -lServiceDiscovery -lDAQStore -L $(ToolFrameworkCore)/lib/ -lToolChain -lMyTools -lDataModelBase -lLogging -lStore -lpthread  $(ToolLibs) -L $(ToolDAQFramework)/lib/ -lToolDAQChain -lDAQDataModelBase  -lDAQLogging -lServiceDiscovery -lDAQStore $(ZMQLib) $(BoostLib)


#.SECONDARY: $(%.o)

all: $(DataModelHEADERS) $(MyToolHEADERS) $(SOURCEFILES) $(LIBRARIES) main NodeDaemon RemoteControl

debug: all

main: src/main.o $(LIBRARIES) $(DataModelHEADERS) $(MyToolHEADERS) | $(SOURCEFILES)
	@echo -e "\e[38;5;11m\n*************** Making " $@ " ****************\e[0m"
	g++  $(CXXFLAGS) $< -o $@ $(Includes) $(Libs) $(DataModelInclude) $(DataModelLib) $(MyToolsInclude) $(MyToolsLib) 

include/%.h:
	@echo -e "\e[38;5;87m\n*************** sym linking headers ****************\e[0m"
	ln -s  `pwd`/$(filter %$(strip $(patsubst include/%.h, /%.h, $@)), $(wildcard DataModel/*.h) $(wildcard UserTools/*/*.h) $(wildcard UserTools/*.h)) $@

src/%.o :  src/%.cpp   
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes)

UserTools/Factory/Factory.o :  UserTools/Factory/Factory.cpp  $(DataModelHEADERS) $(MyToolHEADERS)
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes) $(DataModelInclude) $(ToolsInclude)

UserTools/%.o :  UserTools/%.cpp  $(DataModelHEADERS) UserTools/%.h
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes) $(DataModelInclude) $(ToolsInclude)

DataModel/%.o : DataModel/%.cpp DataModel/%.h  $(DataModelHEADERS)
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes) $(DataModelInclude)

lib/libDataModel.so: $(patsubst %.cpp, %.o , $(wildcard DataModel/*.cpp)) |   $(DataModelHEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes) $(DataModelInclude)

lib/libMyTools.so: $(patsubst %.cpp, %.o , $(filter-out $(AlreadyCompiled), $(wildcard UserTools/*/*.cpp))) |   $(DataModelHEADERS) $(MyToolHEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes) $(DataModelInclude) $(MyToolsInclude)

lib/%.so:
	@echo -e "\e[38;5;87m\n*************** sym linking Tool libs ****************\e[0m"
	ln -s `pwd`/$(filter %$(strip $(patsubst lib/%.so, /%.so ,$@)), $(wildcard UserTools/*/*.so)) $@

NodeDaemon: $(ToolDAQFramework)/NodeDaemon
	@echo -e "\e[38;5;87m\n*************** sym linking " $@ " ****************\e[0m"
	ln -s $(ToolDAQFramework)/NodeDaemon ./

RemoteControl: $(ToolDAQFramework)/RemoteControl
	@echo -e "\e[38;5;87m\n*************** sym linking " $@ " ****************\e[0m"
	ln -s $(ToolDAQFramework)/RemoteControl ./

clean:
	@echo -e "\e[38;5;201m\n*************** Cleaning up ****************\e[0m"
	rm -f */*/*.o
	rm -f */*.o
	rm -f include/*.h
	rm -f lib/*.so
	rm -rf main
	rm -rf NodeDaemon
	rm -rf RemoteControl

Docs:
	doxygen Doxyfile

