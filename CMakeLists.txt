cmake_minimum_required (VERSION 2.6)
project (ToolDAQFramework)

set(ZMQ_INC "${PROJECT_SOURCE_DIR}/../zeromq-4.0.7/include/")
set(ZMQ_LIB_PATH "${PROJECT_SOURCE_DIR}/../zeromq-4.0.7/lib")
set(ZMQ_LIBS zmq)

set(BOOST_INC "${PROJECT_SOURCE_DIR}/../boost_1_66_0/install/include/")
set(BOOST_LIB_PATH "${PROJECT_SOURCE_DIR}/../boost_1_66_0/install/lib")
set(BOOST_LIBS boost_date_time boost_serialization boost_iostreams)

set(DATAMODEL_INC "")
set(DATAMODEL_LIB_PATH "")
set(DATAMODEL_LIBS "")

set(MYTOOLS_INC "")
set(MYTOOLS_LIB_PATH "")
set(MYTOOLS_LIBS "")

set(TOOLFRAMEWORK_INC "${PROJECT_SOURCE_DIR}/../ToolFrameworkCore/include")
set(TOOLFRAMEWORK_LIBS_PATH "${PROJECT_SOURCE_DIR}/../ToolFrameworkCore/lib")
set(TOOLFRAMEWORK_LIBS DataModelBase Logging Store ToolChain)

include_directories (${BOOST_INC} ${ZMQ_INC} ${DATAMODEL_INC} ${MYTOOLS_INC} ${TOOLFRAMEWORK_INC})
link_directories("${PROJECT_SOURCE_DIR}/lib" ${BOOST_LIB_PATH} ${ZMQ_LIB_PATH} ${DATAMODEL_LIB_PATH} ${MYTOOLS_LIB_PATH} ${TOOLFRAMEWORK_LIBS_PATH})

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${PROJECT_SOURCE_DIR}/*.h)
    FILE(COPY ${new_list} DESTINATION ${PROJECT_BINARY_DIR}/include)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

FILE(COPY ${PROJECT_SOURCE_DIR}/configfiles DESTINATION ${PROJECT_BINARY_DIR}/)

HEADER_DIRECTORIES(header_list)
include_directories(${header_list})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

file(GLOB_RECURSE DAQ_STORE_SRC RELATIVE ${CMAKE_SOURCE_DIR} "src/Store/*.cpp")
add_library(DAQStore SHARED ${DAQ_STORE_SRC})

file(GLOB_RECURSE DAQ_LOGGING_SRC RELATIVE ${CMAKE_SOURCE_DIR} "src/DAQLogging/*.cpp")
add_library(DAQLogging SHARED ${DAQ_LOGGING_SRC})

file(GLOB_RECURSE TEMP_DAQ_DATAMODEL_SRC RELATIVE ${CMAKE_SOURCE_DIR} "DataModel/*.cpp")
add_library(TempDAQDataModel SHARED ${TEMP_DAQ_DATAMODEL_SRC})

file(GLOB_RECURSE DAQ_DATAMODEL_BASE_SRC RELATIVE ${CMAKE_SOURCE_DIR} "src/DAQDataModelBase/*.cpp")
add_library(DAQDataModelBase SHARED ${DAQ_DATAMODEL_BASE_SRC})

file(GLOB_RECURSE TEMP_DAQTOOLS_SRC RELATIVE ${CMAKE_SOURCE_DIR} "UserTools/*.cpp")
add_library(TempDAQTools SHARED ${TEMP_DAQTOOLS_SRC})

file(GLOB_RECURSE TOOLDAQCHAIN_SRC RELATIVE ${CMAKE_SOURCE_DIR} "src/ToolDAQChain/*.cpp")
add_library(ToolDAQChain SHARED ${TOOLDAQCHAIN_SRC})

file(GLOB_RECURSE SERVICEDISCOVERY_SRC RELATIVE ${CMAKE_SOURCE_DIR} "src/ServiceDiscovery/*.cpp")
add_library(ServiceDiscovery SHARED ${SERVICEDISCOVERY_SRC})

add_executable (main ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries (main DAQStore DAQLogging ToolDAQChain ServiceDiscovery TempDAQTools TempDAQDataModel DAQDataModelBase ${ZMQ_LIBS} ${BOOST_LIBS} ${DATAMODEL_LIBS} ${MYTOOLS_LIBS} ${TOOLFRAMEWORK_LIBS})

add_executable ( NodeDaemon src/NodeDaemon/NodeDaemon.cpp)
target_link_libraries (NodeDaemon ServiceDiscovery Store ${ZMQ_LIBS} ${BOOST_LIBS})

add_executable ( RemoteControl src/RemoteControl/RemoteControl.cpp)
target_link_libraries (RemoteControl ServiceDiscovery Store ${ZMQ_LIBS} ${BOOST_LIBS})
