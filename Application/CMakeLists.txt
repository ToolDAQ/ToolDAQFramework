#todo sym links not copy headers, use macro to seach for .so files in Usertools and add the libraries to libs list and symlink to libs folder

cmake_minimum_required (VERSION 2.6)

project (ToolDAQApplicaiton)

set(DEPENDENCIES_PATH "${PROJECT_SOURCE_DIR}/Dependencies")

set(ZMQ_INC "${DEPENDENCIES_PATH}/zeromq-4.0.7/include/")
set(ZMQ_LIB_PATH "${DEPENDENCIES_PATH}/zeromq-4.0.7/lib")
set(ZMQ_LIBS zmq)

set(BOOST_INC "${DEPENDENCIES_PATH}/boost_1_66_0/install/include/")
set(BOOST_LIB_PATH "${DEPENDENCIES_PATH}/boost_1_66_0/install/lib")
set(BOOST_LIBS boost_date_time boost_serialization boost_iostreams)

set(DATAMODEL_INC "")
set(DATAMODEL_LIB_PATH "")
set(DATAMODEL_LIBS "")

set(MYTOOLS_INC "")
set(MYTOOLS_LIB_PATH "")
set(MYTOOLS_LIBS "")

set(TOOLFRAMEWORK_INC "${DEPENDENCIES_PATH}/ToolFrameworkCore/include")
set(TOOLFRAMEWORK_LIBS_PATH "${DEPENDENCIES_PATH}/ToolFrameworkCore/lib")
set(TOOLFRAMEWORK_LIBS DataModelBase Logging Store ToolChain)

set(TOOLDAQ_INC "${DEPENDENCIES_PATH}/ToolDAQFramework/include")
set(TOOLDAQ_LIBS_PATH "${DEPENDENCIES_PATH}/ToolDAQFramework/lib")
set(TOOLDAQ_LIBS DAQDataModelBase DAQLogging DAQStore ServiceDiscovery ToolDAQChain)

include_directories (${DATAMODEL_INC} ${MYTOOLS_INC} ${TOOLFRAMEWORK_INC} ${TOOLDAQ_INC} ${ZMQ_INC} ${BOOST_INC})
link_directories(${DATAMODEL_LIB_PATH} ${MYTOOLS_LIB_PATH} ${TOOLFRAMEWORK_LIBS_PATH} ${TOOLDAQ_LIBS_PATH} ${ZMQ_LIB_PATH} ${BOOST_LIB_PATH})

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/DataModel/*.h ${PROJECT_SOURCE_DIR}/UserTools/*.h )
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

file(GLOB_RECURSE DATAMODEL_SRC RELATIVE ${CMAKE_SOURCE_DIR} "DataModel/*.cpp")
add_library(DataModel SHARED ${DATAMODEL_SRC})

file(GLOB_RECURSE MYTOOLS_SRC RELATIVE ${CMAKE_SOURCE_DIR} "UserTools/*.cpp")
add_library(MyTools SHARED ${MYTOOLS_SRC})

add_executable (main ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries (main MyTools DataModel pthread ${DATAMODEL_LIBS} ${MYTOOLS_LIBS} ${TOOLFRAMEWORK_LIBS} ${TOOLDAQ_LIBS} ${ZMQ_LIBS} ${BOOST_LIBS})
