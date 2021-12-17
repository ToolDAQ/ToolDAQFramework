Dependancies=Dependancies

CXXFLAGS=  -fPIC -Wpedantic -O3 # -g -lSegFault -rdynamic -DDEBUG
# -Wl,--no-as-needed

ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS+= -O1 -g -lSegFault -rdynamic -DDEBUG
endif

ZMQLib= -L $(Dependancies)/zeromq-4.0.7/lib -lzmq 
ZMQInclude= -I $(Dependancies)/zeromq-4.0.7/include/ 

BoostLib= -L $(Dependancies)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I $(Dependancies)/boost_1_66_0/install/include

DataModelInclude = 
DataModelLib = 

MyToolsInclude =
MyToolsLib = 

debug: all

all: lib/libStore.so lib/libLogging.so lib/libDAQLogging.so lib/libDataModel.so include/Tool.h lib/libMyTools.so lib/libServiceDiscovery.so lib/libToolChain.so main RemoteControl  NodeDaemon

main: src/main.cpp | lib/libMyTools.so lib/libStore.so lib/libLogging.so lib/libDAQLogging.so lib/libToolChain.so lib/libToolDAQChain.so lib/libDataModel.so lib/libServiceDiscovery.so
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) src/main.cpp -o main -I include -L lib -lStore -lMyTools -lToolChain -lToolDAQChain -lDataModel -lLogging -lDAQLogging -lServiceDiscovery -lpthread $(DataModelInclude) $(DataModelLib) $(MyToolsInclude)  $(MyToolsLib) $(ZMQLib) $(ZMQInclude)  $(BoostLib) $(BoostInclude)


lib/libStore.so: $(Dependancies)/ToolDAQFramework/src/Store/* $(Dependancies)/ToolFrameworkCore/src/Store/*
	cd $(Dependancies)/ToolDAQFramework && $(MAKE) lib/libStore.so
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolDAQFramework/src/Store/*.h include/
	cp $(Dependancies)/ToolFrameworkCore/src/Store/*.h include/
	cp $(Dependancies)/ToolDAQFramework/lib/libStore.so lib/


include/Tool.h:  $(Dependancies)/ToolFrameworkCore/src/Tool/Tool.h
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolFrameworkCore/src/Tool/Tool.h include/


lib/libToolChain.so: $(Dependancies)/ToolFrameworkCore/src/ToolChain/* | lib/libLogging.so lib/libDAQLogging.so lib/libStore.so lib/libMyTools.so lib/libServiceDiscovery.so lib/libLogging.so lib/libDAQLogging.so lib/libDataModel.so
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	cp $(Dependancies)/ToolFrameworkCore/src/ToolChain/ToolChain.h include/
	g++ $(CXXFLAGS) -shared $(Dependancies)/ToolFrameworkCore/src/ToolChain/ToolChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lServiceDiscovery -lLogging -lDAQLogging -lMyTools -o lib/libToolChain.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(MyToolsInclude) $(MyToolsLib) $(BoostLib) $(BoostInclude)

lib/libToolDAQChain.so:  $(Dependancies)/ToolDAQFramework/src/ToolDAQChain/* | lib/libToolChain.so lib/libLogging.so lib/libDAQLogging.so lib/libStore.so lib/libMyTools.so lib/libServiceDiscovery.so lib/libLogging.so lib/libDAQLogging.so lib/libDataModel.so
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	cp $(Dependancies)/ToolDAQFramework/src/ToolDAQChain/*.h include/
	g++ $(CXXFLAGS) -shared $(Dependancies)/ToolDAQFramework/src/ToolDAQChain/ToolDAQChain.cpp -I include -lpthread -L lib -lStore -lDataModel -lServiceDiscovery -lLogging -lDAQLogging -lMyTools -lToolChain -o lib/libToolDAQChain.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(MyToolsInclude) $(MyToolsLib) $(BoostLib) $(BoostInclude)

clean: 
	@echo -e "\e[38;5;201m\n*************** Cleaning up ****************\e[0m"
	rm -f include/*.h
	rm -f lib/*.so
	rm -f main
	rm -f RemoteControl
	rm -f NodeDaemon
	rm -f UserTools/*/*.o
	rm -f DataModel/*.o

lib/libDataModel.so: DataModel/* lib/libLogging.so lib/libDAQLogging.so lib/libStore.so $(patsubst DataModel/%.cpp, DataModel/%.o, $(wildcard DataModel/*.cpp))
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -shared DataModel/*.o -I include -L lib -lStore -lLogging -lDAQLogging -o lib/libDataModel.so $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

lib/libMyTools.so: UserTools/*/* UserTools/* include/Tool.h  lib/libLogging.so lib/libDAQLogging.so lib/libStore.so UserTools/Factory/Factory.o |lib/libDataModel.so
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	g++ $(CXXFLAGS) -shared UserTools/*/*.o -I include -L lib -lStore -lDataModel -lLogging -lDAQLogging -o lib/libMyTools.so $(MyToolsInclude) $(DataModelInclude) $(MyToolsLib) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

RemoteControl: $(Dependancies)/ToolDAQFramework/src/RemoteControl/* lib/libServiceDiscovery.so lib/libStore.so
	cd $(Dependancies)/ToolDAQFramework/ && $(MAKE) RemoteControl
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolDAQFramework/RemoteControl ./

NodeDaemon: $(Dependancies)/ToolDAQFramework/src/NodeDaemon/* lib/libServiceDiscovery.so lib/libStore.so
	cd $(Dependancies)/ToolDAQFramework/ && $(MAKE) NodeDaemon
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolDAQFramework/NodeDaemon ./

lib/libServiceDiscovery.so: $(Dependancies)/ToolDAQFramework/src/ServiceDiscovery/* | lib/libStore.so
	cd $(Dependancies)/ToolDAQFramework && $(MAKE) lib/libServiceDiscovery.so
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolDAQFramework/src/ServiceDiscovery/ServiceDiscovery.h include/
	cp $(Dependancies)/ToolDAQFramework/lib/libServiceDiscovery.so lib/

lib/libLogging.so:  $(Dependancies)/ToolFrameworkCore/src/Logging/* | lib/libStore.so
	cd $(Dependancies)/ToolDAQFramework && $(MAKE) lib/libLogging.so
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolFrameworkCore/src/Logging/Logging.h include/
	cp $(Dependancies)/ToolDAQFramework/lib/libLogging.so lib/

lib/libDAQLogging.so:  $(Dependancies)/ToolDAQFramework/src/DAQLogging/* | lib/libStore.so
	cd $(Dependancies)/ToolDAQFramework && $(MAKE) lib/libDAQLogging.so
	@echo -e "\e[38;5;118m\n*************** Copying " $@ "****************\e[0m"
	cp $(Dependancies)/ToolDAQFramework/src/DAQLogging/DAQLogging.h include/
	cp $(Dependancies)/ToolDAQFramework/lib/libDAQLogging.so lib/

update:
	@echo -e "\e[38;5;51m\n*************** Updating ****************\e[0m"
	cd $(Dependancies)/ToolFrameworkCore; git pull
	cd $(Dependancies)/ToolDAQFramework; git pull
	cd $(Dependancies)/zeromq-4.0.7; git pull
	git pull

UserTools/Factory/Factory.o: UserTools/Factory/Factory.cpp lib/libStore.so include/Tool.h lib/libDAQLogging.so lib/libDataModel.so $(patsubst UserTools/%.cpp, UserTools/%.o, $(wildcard UserTools/*/*.cpp)) | include/Tool.h
	@echo -e "\e[38;5;214m\n*************** Making " $@ "****************\e[0m"
	cp UserTools/Factory/Factory.h include
	cp UserTools/Unity.h include
	-g++ $(CXXFLAGS) -c -o $@ $< -I include -L lib -lStore -lDataModel -lLogging -lDAQLogging $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)


UserTools/%.o: UserTools/%.cpp lib/libStore.so include/Tool.h lib/libLogging.so lib/libDAQLogging.so lib/libDataModel.so | include/Tool.h
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	cp $(shell dirname $<)/*.h include
	-g++ -c $(CXXFLAGS) -o $@ $< -I include -L lib -lStore -lDataModel -lLogging -lDAQLogging $(MyToolsInclude) $(MyToolsLib) $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)

target: remove $(patsubst %.cpp, %.o, $(wildcard UserTools/$(TOOL)/*.cpp))

remove:
	echo -e "removing"
	-rm UserTools/$(TOOL)/*.o

DataModel/%.o: DataModel/%.cpp lib/libLogging.so lib/libDAQLogging.so lib/libStore.so
	@echo -e "\e[38;5;226m\n*************** Making " $@ "****************\e[0m"
	cp $(shell dirname $<)/*.h include
	-g++ -c $(CXXFLAGS) -o $@ $< -I include -L lib -lStore -lLogging -lDAQLogging  $(DataModelInclude) $(DataModelLib) $(ZMQLib) $(ZMQInclude) $(BoostLib) $(BoostInclude)


Docs:
	doxygen Doxyfile
