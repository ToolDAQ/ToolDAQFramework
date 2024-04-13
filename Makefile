ToolFrameworkDIR=../ToolFrameworkCore
SOURCEDIR=`pwd`

CXXFLAGS=  -fPIC -O3 -Wpedantic -Wall -std=c++11 -Wno-comment #-Wno-unused -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept  -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef #-Werror -Wold-style-cast 

ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS+= -O0 -g -lSegFault -rdynamic -DDEBUG
endif

ZMQLib= -L ../zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I ../zeromq-4.0.7/include/ 

BoostLib= -L ../boost_1_66_0/install/lib -lboost_date_time -lboost_serialization  -lboost_iostreams
BoostInclude= -I ../boost_1_66_0/install/include/

TempDataModelInclude =
TempDataModelLib =

TempToolsInclude =
TempToolsLib =


Includes=  -I $(ToolFrameworkDIR)/include/ -I $(SOURCEDIR)/include/  -I $(SOURCEDIR)/tempinclude/ $(ZMQInclude) $(BoostInclude) 
Libs=-L $(SOURCEDIR)/lib/ -lTempDAQDataModel -lTempDAQTools -lToolDAQChain -lDAQDataModelBase -lDAQStore -lServiceDiscovery -lDAQLogging  $(BoostLib) $(ZMQLib) -L $(ToolFrameworkDIR)/lib/  -lToolChain -lTempDAQTools -lDataModelBase  -lpthread -lLogging -lStore
LIBRARIES=lib/libDAQStore.so lib/libDAQLogging.so lib/libToolDAQChain.so lib/libDAQDataModelBase.so lib/libTempDAQDataModel.so lib/libTempDAQTools.so lib/libServiceDiscovery.so
HEADERS:=$(patsubst %.h, include/%.h, $(filter %.h, $(subst /, ,$(wildcard src/*/*.h) )))
TempDataModelHEADERS:=$(patsubst %.h, tempinclude/%.h, $(filter %.h, $(subst /, , $(wildcard DataModel/*.h))))
TempToolHEADERS:=$(patsubst %.h, tempinclude/%.h, $(filter %.h, $(subst /, , $(wildcard UserTools/*/*.h) $(wildcard UserTools/*.h))))
SOURCEFILES:=$(patsubst %.cpp, %.o, $(wildcard */*.cpp) $(wildcard */*/*.cpp))

#.SECONDARY: $(%.o)


all: $(HEADERS) $(TempDataModelHEADERS) $(TempToolHEADERS) $(SOURCEFILES) $(LIBRARIES) main NodeDaemon RemoteControl

debug: all

main: src/main.o $(LIBRARIES) $(HEADERS) $(TempDataModelHEADERS) $(TempToolHEADERS) | $(SOURCEFILES)
	@echo -e "\e[38;5;11m\n*************** Making " $@ " ****************\e[0m"
	g++  $(CXXFLAGS) $< -o $@ $(Includes) $(Libs) $(TempDataModelInclude) $(TempDataModelLib) $(TempToolsInclude) $(TempTooLsLib) 

include/%.h:
	@echo -e "\e[38;5;87m\n*************** sym linking headers ****************\e[0m"
	ln -s  $(SOURCEDIR)/$(filter %$(strip $(patsubst include/%.h, /%.h, $@)), $(wildcard src/*/*.h) ) $@

tempinclude/%.h:
	@echo -e "\e[38;5;87m\n*************** sym linking headers ****************\e[0m"
	ln -s  $(SOURCEDIR)/$(filter %$(strip $(patsubst tempinclude/%.h, /%.h, $@)), $(wildcard DataModel/*.h) $(wildcard UserTools/*/*.h) $(wildcard UserTools/*.h)) $@

src/%.o :  src/%.cpp $(HEADERS)  
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes)

UnitTests/%.o : UnitTests/%.cpp $(HEADERS) 
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes)

UserTools/%.o :  UserTools/%.cpp $(HEADERS) $(TempDataModelHEADERS) UserTools/%.h
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes) $(TempDataModelInclude) $(TempToolsInclude)

UserTools/Factory/Factory.o :  UserTools/Factory/Factory.cpp $(HEADERS) $(TempDataModelHEADERS) 
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes) $(TempDataModelInclude) $(TempToolsInclude)

DataModel/%.o : DataModel/%.cpp DataModel/%.h $(HEADERS) $(TempDataModelHEADERS)
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -c $< -o $@ $(Includes) $(TempDataModelInclude)

lib/libDAQStore.so: $(patsubst %.cpp, %.o , $(wildcard src/Store/*.cpp)) | $(HEADERS) 
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes)

lib/libDAQLogging.so: $(patsubst %.cpp, %.o , $(wildcard src/DAQLogging/*.cpp)) | $(HEADERS) 
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes)

lib/libServiceDiscovery.so: $(patsubst %.cpp, %.o , $(wildcard src/ServiceDiscovery/*.cpp)) | $(HEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes)

lib/libDAQDataModelBase.so: $(patsubst %.cpp, %.o , $(wildcard src/DAQDataModelBase/*.cpp)) | $(HEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes) 

lib/libToolDAQChain.so: $(patsubst %.cpp, %.o , $(wildcard src/ToolDAQChain/*.cpp)) | $(HEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes)

lib/libTempDAQDataModel.so: $(patsubst %.cpp, %.o , $(wildcard DataModel/*.cpp)) | $(HEADERS) $(TempDataModelHEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes) $(TempDataModelInclude)

lib/libTempDAQTools.so: $(patsubst %.cpp, %.o , $(wildcard UserTools/*/*.cpp)) | $(HEADERS) $(TempDataModelHEADERS) $(TempToolHEADERS)
	@echo -e "\e[38;5;201m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) --shared $^ -o $@ $(Includes) $(TempDataModelInclude) $(TempToolsInclude)

NodeDaemon: src/NodeDaemon/NodeDaemon.o $(LIBRARIES) $(HEADERS) | $(SOURCEFILES)
	@echo -e "\e[38;5;11m\n*************** Making " $@ " ****************\e[0m"
	g++  $(CXXFLAGS) $< -o $@ $(Includes) $(Libs)  

RemoteControl: src/RemoteControl/RemoteControl.o $(LIBRARIES) $(HEADERS) | $(SOURCEFILES)
	@echo -e "\e[38;5;11m\n*************** Making " $@ " ****************\e[0m"
	g++  $(CXXFLAGS) $< -o $@ $(Includes) $(Libs)  

clean:
	@echo -e "\e[38;5;201m\n*************** Cleaning up ****************\e[0m"
	rm -f */*/*.o
	rm -f */*.o
	rm -f include/*.h
	rm -f tempinclude/*.h
	rm -f lib/*.so
	rm -f main
	rm -f NodeDaemon
	rm -f RemoteControl

Docs:
	doxygen Doxyfile
