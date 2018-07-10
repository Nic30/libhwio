cmake_minimum_required(VERSION 3.0)
project(hwio)

include(GNUInstallDirs)

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
	./src/bus/hwio_bus_xml.h
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
	./src/bus/hwio_bus_xml.cpp
)
find_package(LibXml2 REQUIRED)

# define main library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_library(hwio STATIC ${LIB_HWIO_INCLUDE} ${LIB_HWIO_SRC})
target_include_directories(hwio
	PUBLIC ${LIBXML2_INCLUDE_DIR}
	PUBLIC ${LIB_HWIO_PRIVATE_INCLUDE_DIRS})
target_link_libraries(hwio 
	PUBLIC ${LIBXML2_LIBRARIES})

set(LIB_HWIO_TEST_SRC 
	./src/selftest/test_hwio_bus_remote.cpp
	./src/selftest/test_hwio_bus_xml.cpp
	./src/selftest/test_hwio_version.cpp
	./src/selftest/hwio_selftest.cpp
	./src/selftest/hwio_test.h
	./src/selftest/test_hwio_comp_spec.cpp
	./src/selftest/test_hwio_component_search.cpp
	./src/selftest/test_hwio_bus_devicetree.cpp
)

# define testsuite executable
add_executable(libhwio_test ${LIB_HWIO_INCLUDE} ${LIB_HWIO_SRC} ${LIB_HWIO_TEST_SRC})
target_include_directories(libhwio_test PRIVATE ${LIBXML2_INCLUDE_DIR} ${LIB_HWIO_PRIVATE_INCLUDE_DIRS})

# Add pthread library
find_package(Threads REQUIRED)
target_link_libraries(libhwio_test
	PUBLIC Threads::Threads
	PUBLIC ${LIBXML2_LIBRARIES}
)


# Define install files
set(headers ${LIB_HWIO_INCLUDE})

install(TARGETS hwio
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS hwio EXPORT hwio-targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT hwio-targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/hwio)
install(FILES ${headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hwio")
install(FILES hwio-config.cmake DESTINATION "${CMAKE_INSTALL_LIBDIR}/hwio")

