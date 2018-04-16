# Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
# This file is part of EngineCL which is released under MIT License.
# See file LICENSE for full license details.

message("src: ${CMAKE_CURRENT_SOURCE_DIR}")
message("root_dir: ${ROOT_DIR}")
message("src_dir: ${SRC_DIR}")
message("deps_dir: ${DEPS_DIR}")
message("include_dir: ${INCLUDE_DIR}")

set(Entry "enginecl-base")
set(Base_Library "${Entry}" PARENT_SCOPE)

set (Needed_Libraries
#  "<CL/cl.h>"
  "<pthread.h>"
  "<omp.h>"
  # "<math.h>"
)


# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

include_directories ("${SRC_DIR}")
include_directories ("${DEPS_DIR}")
include(SetLibraries)

set(SOURCES
  buffer.cpp
  runtime.cpp
  schedulers/static.cpp
  schedulers/dynamic.cpp
  schedulers/proportional.cpp
  schedulers/hguided.cpp
  device.cpp
  clutils.cpp
  inspector.cpp
)

set(HEADERS
  ${INCLUDE_DIR}/config.hpp
  ${INCLUDE_DIR}/buffer.hpp
  ${INCLUDE_DIR}/EngineCL.hpp
  ${INCLUDE_DIR}/semaphore.hpp
  ${INCLUDE_DIR}/runtime.hpp
  ${INCLUDE_DIR}/schedulers/static.hpp
  ${INCLUDE_DIR}/schedulers/dynamic.hpp
  ${INCLUDE_DIR}/schedulers/proportional.hpp
  ${INCLUDE_DIR}/schedulers/hguided.hpp
  ${INCLUDE_DIR}/scheduler.hpp
  ${INCLUDE_DIR}/device.hpp
  ${INCLUDE_DIR}/clutils.hpp
  ${INCLUDE_DIR}/inspector.hpp
)

IncludeLibraries("${Entry}" "${Needed_Libraries}")

# add_library("${Entry}" ${SOURCES} ${HEADERS})
add_library("${Entry}" SHARED ${HEADERS} ${SOURCES})

LinkLibraries("${Entry}" "${Needed_Libraries}")

# target_link_libraries("${Entry}" "${CONAN_LIBS}")

# Tools

set(clkernel_Entry clkernel)

set (clkernel_Needed_Libraries
 # "<CL/cl.h>"
)

set(clkernel_SOURCES
  tools/clkernel.cpp
  clutils.cpp
)

IncludeLibraries("${clkernel_Entry}" "${clkernel_Needed_Libraries}")

add_executable("${clkernel_Entry}" EXCLUDE_FROM_ALL ${clkernel_SOURCES})

set(EXECUTABLE_OUTPUT_PATH "../tools")

LinkLibraries("${clkernel_Entry}" "${clkernel_Needed_Libraries}")
