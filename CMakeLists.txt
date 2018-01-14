cmake_minimum_required(VERSION 3.3)

set(PROJECT clbalancer)
set(EXECUTABLE clbalancer)
project(${PROJECT})

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR "${ROOT_DIR}/src")
set(INCLUDE_DIR "${ROOT_DIR}/include")
set(DEPS_DIR "${ROOT_DIR}/deps")

message("root_dir: ${ROOT_DIR}")
message("src_dir: ${ROOT_DIR}")

list(APPEND CMAKE_MODULE_PATH "${ROOT_DIR}/cmake/modules")

set (Needed_Libraries
  "<CL/cl.h>"
  "<pthread.h>"
  "<omp.h>"
  "<math.h>"
)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

include(OpenCL)
include(SetLibraries)
include(FindNode)
include(SetFlags)
# base library

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

configOpenCL()
message("OPENCL_VERSION: ${OPENCL_VERSION}")
# find_package(Qt5Core REQUIRED)

include_directories("/usr/include/qt")
# find_package(Qt5 COMPONENTS Core REQUIRED)

# add_definitions(${Qt5Core_DEFINITIONS})
# include_directories(${Qt5Core_INCLUDES})
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")
# target_link_libraries(hello_world ${Qt5Widgets_LIBRARIES})

FindNode()
message("NODE: ${NODE}")
# message("After FindNode() NODE: ${NODE}")

SetFlags()

include_directories ("${SRC_DIR}")
include_directories ("${DEPS_DIR}")

# add_subdirectory(deps)
# message("Deps_Library: ${Deps_Library}")

add_subdirectory(src)
message("Base_Library: ${Base_Library}")

add_subdirectory(benchsuite)
message("Benchsuite_Library: ${Benchsuite_Library}")

set(Entry "clbalancer")
add_executable("${Entry}" benchsuite/main.cpp)
target_link_libraries("${Entry}"
  "${Base_Library}"
  "${Benchsuite_Library}"
  "${Deps_Library}"
  # Qt5::Core
)
LinkLibraries("${Entry}" "${Needed_Libraries}")

# link_directories("${SRC_DIR}")
# add_executable("${Entry}" src/device.hpp src/main.cpp)
# IncludeLibraries("${Entry}" "${Needed_Libraries}")

# target_link_libraries("${Base_Library}" "${CONAN_LIBS}")
# target_link_libraries("${Entry}" "${Base_Library}" "${CONAN_LIBS}")


# add_subdirectory(support/src/semaphore)

# tests

# enable_testing()
# add_subdirectory(test)
