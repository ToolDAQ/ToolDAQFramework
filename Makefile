ToolFrameworkPath= ../ToolFrameworkCore

CXXFLAGS=  -fPIC -O3 -Wpedantic

ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS+= -O1 -g -lSegFault -rdynamic -DDEBUG
endif

ZMQLib= -L ../zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I ../zeromq-4.0.7/include/ 

BoostLib= -L ../boost_1_66_0/install/lib -lboost_date_time -lboost_serialization  -lboost_iostreams
BoostInclude= -I ../boost_1_66_0/install/include/

DataModelInclude =
DataModelLib =

MyToolsInclude =
MyToolsLib =

all: lib/libMyTools.so lib/libToolDAQChain.so lib/libToolChain.so lib/libStore.so include/Tool.h lib/libServiceDiscovery.so lib/libDataModel.so lib/libLogging.so lib/libDAQLogging.so RemoteControl NodeDaemon main

main: src/main.cpp lib/libStore.so lib/libLogging.so lib/libDAQLogging.so lib/libToolDAQChain.so lib/libToolChain.so lib/libServiceDiscovery.so | lib/libMyTools.so  lib/libDataModel.so 
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) src/main.cpp -o main -I include -L lib -lStore -lMyTools -lToolChain -lToolDAQChain -lDataModel -lLogging -lDAQLogging -lServiceDiscovery -lpthread $(DataModelInclude) $(MyToolsInclude) $(MyToolsLib) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)


lib/libStore.so: src/Store/*  $(ToolFrameworkPath)/src/Store/*

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp src/Store/*.h include/
	cp $(ToolFrameworkPath)/src/Store/*.h include/
	g++ $(CXXFLAGS) -shared -I include src/Store/*.cpp  $(ToolFrameworkPath)/src/Store/*.cpp -o lib/libStore.so $(BoostLib) $(BoostInclude)


include/Tool.h: $(ToolFrameworkPath)/src/Tool/Tool.h

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp $(ToolFrameworkPath)/src/Tool/Tool.h include/

lib/libToolChain.so: $(ToolFrameworkPath)/src/ToolChain/* lib/libStore.so include/Tool.h lib/libServiceDiscovery.so lib/libLogging.so lib/libDAQLogging.so |  lib/libDataModel.so lib/libMyTools.so 

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp $(ToolFrameworkPath)/src/ToolChain/ToolChain.h include/
	g++ $(CXXFLAGS) -shared $(ToolFrameworkPath)/src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lMyTools -lServiceDiscovery -lLogging -o lib/libToolChain.so $(DataModelInclude) $(MyToolsInclude) $(MyToolsLib) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

lib/libToolDAQChain.so: src/ToolDAQChain/* lib/libStore.so include/Tool.h lib/libServiceDiscovery.so lib/libLogging.so lib/libDAQLogging.so lib/libToolChain.so|  lib/libDataModel.so lib/libMyTools.so

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp src/ToolDAQChain/ToolDAQChain.h include/
	g++ $(CXXFLAGS) -shared src/ToolDAQChain/ToolDAQChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lMyTools -lServiceDiscovery -lLogging -lDAQLogging -lToolChain -o lib/libToolDAQChain.so $(DataModelInclude) $(MyToolsInclude) $(MyToolsLib) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

clean: 

	@echo -e "\e[38;5;201m\n*************** Cleaning up ****************\e[0m"
	rm -f include/*.h
	rm -f lib/*.so
	rm -f main
	rm -f RemoteControl
	rm -f NodeDaemon
	rm -f UserTools/*/*.o
	rm -f DataModel/*.o

include/Utilities.h: $(ToolFrameworkPath)/DataModel/Utilities.h
	@echo -e "\e[38;5;214m\n*************** Copying " $@ "****************\e[0m"
	cp $(ToolFrameworkPath)/DataModel/Utilities.h ./include/

lib/libDataModel.so: DataModel/* lib/libLogging.so  lib/libDAQLogging.so lib/libStore.so  $(ToolFrameworkPath)/DataModel/Utilities.cpp $(patsubst DataModel/%.cpp, DataModel/%.o, $(wildcard DataModel/*.cpp))

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp $(ToolFrameworkPath)/DataModel/Utilities.h ./include/
	g++ $(CXXFLAGS) -shared DataModel/*.o $(ToolFrameworkPath)/DataModel/Utilities.cpp -I include -L lib -lStore -lLogging -lDAQLogging -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)


lib/libMyTools.so: UserTools/*/* UserTools/* lib/libStore.so include/Tool.h lib/libLogging.so lib/libDAQLogging.so UserTools/Factory/Factory.o | lib/libDataModel.so 

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -shared UserTools/*/*.o -I include -L lib -lStore -lDataModel -lLogging -lDAQLogging -o lib/libMyTools.so $(MyToolsInclude) $(DataModelInclude) $(MyToolsLib) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

RemoteControl: src/RemoteControl/* lib/libStore.so lib/libServiceDiscovery.so

	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) src/RemoteControl/RemoteControl.cpp -o RemoteControl -I include -L lib -lStore -lServiceDiscovery $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

NodeDaemon: src/NodeDaemon/* lib/libStore.so lib/libServiceDiscovery.so
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) src/NodeDaemon/NodeDaemon.cpp -o NodeDaemon -I ./include/ -L ./lib/ -lServiceDiscovery -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libServiceDiscovery.so: src/ServiceDiscovery/* lib/libStore.so
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp src/ServiceDiscovery/ServiceDiscovery.h include/
	g++ $(CXXFLAGS) -shared -I include src/ServiceDiscovery/ServiceDiscovery.cpp -o lib/libServiceDiscovery.so -L lib/ -lStore  $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libLogging.so:  $(ToolFrameworkPath)/src/Logging/* lib/libStore.so 
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp $(ToolFrameworkPath)/src/Logging/Logging.h include/
	g++ $(CXXFLAGS) -shared -I include $(ToolFrameworkPath)/src/Logging/Logging.cpp -o lib/libLogging.so -L lib/ -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

lib/libDAQLogging.so: src/DAQLogging/*  lib/libStore.so 
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp src/DAQLogging/DAQLogging.h include/
	g++ $(CXXFLAGS) -shared -I include src/DAQLogging/DAQLogging.cpp -o lib/libDAQLogging.so -L lib/ -lLogging -lStore $(ZMQInclude) $(ZMQLib) $(BoostLib) $(BoostInclude)

UserTools/Factory/Factory.o: UserTools/Factory/Factory.cpp lib/libStore.so include/Tool.h lib/libDAQLogging.so lib/libDataModel.so $(patsubst UserTools/%.cpp, UserTools/%.o, $(wildcard UserTools/*/*.cpp)) | include/Tool.h
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp UserTools/Factory/Factory.h include
	cp UserTools/Unity.h include
	-g++ $(CXXFLAGS) -c -o $@ $< -I include -L lib -lStore -lDataModel -lLogging -lDAQLogging $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

UserTools/%.o: UserTools/%.cpp lib/libStore.so include/Tool.h lib/libLogging.so  lib/libDAQLogging.so lib/libDataModel.so | include/Tool.h
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp $(shell dirname $<)/*.h include
	-g++ $(CXXFLAGS) -c -o $@ $< -I include -L lib -lStore -lDataModel -lLogging -lDAQLogging $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

target: remove $(patsubst %.cpp, %.o, $(wildcard UserTools/$(TOOL)/*.cpp))

remove:
	echo "removing"
	-rm UserTools/$(TOOL)/*.o

DataModel/%.o: DataModel/%.cpp lib/libLogging.so lib/libDAQLogging.so lib/libStore.so include/Utilities.h
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp $(shell dirname $<)/*.h include
	-g++ $(CXXFLAGS) -c -o $@ $< -I include -L lib -lStore -lLogging -lDAQLogging  $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

Docs:
	doxygen Doxyfile
