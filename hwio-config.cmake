get_filename_component(hwio_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${hwio_DIR}/hwio-targets.cmake)
get_filename_component(hwio_INCLUDE_DIRS "${hwio_DIR}/../../include/hwio" ABSOLUTE)
