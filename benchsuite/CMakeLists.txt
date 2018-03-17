message("src: ${CMAKE_CURRENT_SOURCE_DIR}")
message("root_dir: ${ROOT_DIR}")
message("src_dir: ${SRC_DIR}")
message("deps_dir: ${DEPS_DIR}")

set(Entry "benchsuite-core")
set(Benchsuite_Library "${Entry}" PARENT_SCOPE)

set (Needed_Libraries
#  "<CL/cl.h>"
  "<pthread.h>"
  "<omp.h>"
  "<math.h>"
)

# include(SetFlags)

# include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

include_directories ("${SRC_DIR}")
# include_directories ("${DEPS_DIR}")
include(SetLibraries)

set(SOURCES
  assign.cpp
  vecadd.cpp
  gaussian.cpp
  ray.cpp
  mersenne.cpp
  sobel.cpp
  parse_ppm.cpp
  watermarking.cpp
  matrix_mult.cpp
  aes_proc.cpp
  aes_decrypt.cpp
  utils/io.cpp
)

set(HEADERS
  assign.hpp
  vecadd.hpp
  gaussian.hpp
  ray.hpp
  mersenne.hpp
  sobel.hpp
  watermarking.hpp
  matrix_mult.hpp
  parse_ppm.hpp
  aes_proc.hpp
  aes_decrypt.hpp
  utils/io.hpp
)

IncludeLibraries("${Entry}" "${Needed_Libraries}")

# add_library("${Entry}" ${SOURCES} ${HEADERS})
add_library("${Entry}" SHARED ${SOURCES} ${HEADERS})

LinkLibraries("${Entry}" "${Needed_Libraries}")

target_link_libraries("${Entry}" "${Base_Library}")
# target_link_libraries("${Entry}" "${CONAN_LIBS}")
