message("src: ${CMAKE_CURRENT_SOURCE_DIR}")
message("root_dir: ${ROOT_DIR}")
message("src_dir: ${SRC_DIR}")
message("deps_dir: ${DEPS_DIR}")

set(Entry "clbalancer-base")
set(Base_Library "${Entry}" PARENT_SCOPE)

set (Needed_Libraries
  "<CL/cl.h>"
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
  schedulers/hguided.cpp
  device.cpp
  clutils.cpp
)

set(HEADERS
  buffer.hpp
  clbalancer.hpp
  semaphore.hpp
  runtime.hpp
  schedulers/static.hpp
  schedulers/dynamic.hpp
  schedulers/hguided.hpp
  scheduler.hpp
  device.hpp
  clutils.hpp
)

IncludeLibraries("${Entry}" "${Needed_Libraries}")

# add_library("${Entry}" ${SOURCES} ${HEADERS})
add_library("${Entry}" SHARED ${SOURCES} ${HEADERS})

LinkLibraries("${Entry}" "${Needed_Libraries}")

# target_link_libraries("${Entry}" "${CONAN_LIBS}")
