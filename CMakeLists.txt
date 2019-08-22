cmake_minimum_required(VERSION 3.0)
project(hwio)

include(GNUInstallDirs)
include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(LIB_HWIO_INCLUDE 
	./src/hwio.h
	./src/hwio_cli.h
	./src/hwio_remote.h
	./src/hwio_typedefs.h
	./src/device/ihwio_dev.h
	./src/device/hwio_device_mmap.h
	./src/device/hwio_device_remote.h
	./src/hwio_comp_spec.h
	./src/hwio_remote_utils.h
	./src/server/hwio_server.h
	./src/hwio_version.h
	./src/bus/hwio_bus_remote.h
	./src/bus/hwio_bus_devicetree.h
	./src/bus/hwio_bus_primitive.h
	./src/bus/hwio_bus_composite.h
	./src/bus/hwio_bus_json.h
	./src/bus/ihwio_bus.h
	./src/bus/hwio_client_to_server_con.h
)

set(LIB_HWIO_PRIVATE_INCLUDE_DIRS
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/bus/>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/device/>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/server/>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/selftest/> 
)

set(LIB_HWIO_SRC
	./src/hwio_cli.cpp
	./src/hwio_remote_utils.cpp
	./src/hwio_version.cpp
	./src/device/hwio_device_mmap.cpp
	./src/device/hwio_device_remote.cpp
	./src/server/hwio_server.cpp
	./src/server/hwio_server_utils.cpp
	./src/server/hwio_server_rw.cpp
	./src/server/hwio_server_query.cpp
	./src/server/hwio_server_remote_call.cpp
	./src/hwio_comp_spec.cpp
	./src/bus/hwio_bus_primitive.cpp
	./src/bus/hwio_client_to_server_con.cpp
	./src/bus/hwio_bus_remote.cpp
	./src/bus/hwio_bus_devicetree.cpp
	./src/bus/hwio_bus_composite.cpp
	./src/bus/hwio_bus_json.cpp
)
find_package(Boost REQUIRED)
# define main library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(hwio STATIC ${LIB_HWIO_INCLUDE} ${LIB_HWIO_SRC})
target_include_directories(hwio
	PUBLIC ${Boost_INCLUDE_DIRS}
	PUBLIC ${LIB_HWIO_PRIVATE_INCLUDE_DIRS})

# Define install files
set(headers ${LIB_HWIO_INCLUDE})

install(TARGETS hwio
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS hwio EXPORT hwio-targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT hwio-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/hwio)
install(FILES ${headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hwio")
install(FILES hwio-config.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/hwio")

SET(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-dev")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Michal Orsak <michal.o.socials@gmail.com>")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
INCLUDE(CPack)

################################### TESTS ##################################
if(CMAKE_TESTING_ENABLED)
	find_package(Boost REQUIRED COMPONENTS unit_test_framework)
	find_package(Threads REQUIRED)
		
	file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.cpp)
	
	#Run through each source
	foreach(testSrc ${TEST_SRCS})
	
	    #Extract the filename without an extension (NAME_WE)
	    get_filename_component(testName ${testSrc} NAME_WE)
	
	    #Add compile target
	    add_executable(${testName} ${LIB_HWIO_INCLUDE} ${testSrc})
	    # indicates the shared library variant
		target_include_directories(${testName}
			PRIVATE ${Boost_INCLUDE_DIRS}
			 		${LIB_HWIO_PRIVATE_INCLUDE_DIRS}
			 		${CMAKE_CURRENT_SOURCE_DIR}/test)
		target_compile_definitions(${testName} PRIVATE "BOOST_TEST_DYN_LINK=1")
	    target_link_libraries(${testName} hwio ${Boost_LIBRARIES} Threads::Threads)
	
	    #Move testing binaries into a testBin directory
	    set_target_properties(${testName} PROPERTIES 
	        RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/testBin)
	
	    #Finally add it to test execution - 
	    #Notice the WORKING_DIRECTORY and COMMAND
	    add_test(NAME ${testName} 
	             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ 
	             COMMAND ${CMAKE_CURRENT_BINARY_DIR}/testBin/${testName} )
	endforeach(testSrc)
endif()


