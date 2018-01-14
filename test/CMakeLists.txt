cmake_minimum_required(VERSION 3.3)

set(TESTS
  semaphore.cpp
  # src/string.cpp
  # test/semaphore.cpp
  # test/process.cpp
  # test/config.cpp
  # test/args.cpp
  # test/tests.cpp
  tests.cpp
)

set(TestExecutable "test-${EXECUTABLE}")

message("test: ${CMAKE_CURRENT_SOURCE_DIR}")
message("src: ${SRC_DIR}")
message("root_dir: ${ROOT_DIR}")

include_directories("${SRC_DIR}")
# include_directories("${ROOT_DIR}/include")

# add_library(${BASE_LIBRARY} ${SOURCES} ${HEADERS})

add_executable(${TestExecutable} ${TESTS} ${HEADERS})

target_link_libraries(${TestExecutable} ${Base_Library})

LinkLibraries("${TestExecutable}" "${Needed_Libraries}")

# find_package (Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
# include_directories (${TEST_SOURCE_DIR}/src
#                      ${Boost_INCLUDE_DIRS}
#                      )
# add_definitions (-DBOOST_TEST_DYN_LINK)
# add_executable (Test test.cpp)
# target_link_libraries (Test
#                        Sqr
#                        ${Boost_FILESYSTEM_LIBRARY}
#                        ${Boost_SYSTEM_LIBRARY}
#                        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
#                        )

add_test(NAME MyTest COMMAND ${TestExecutable})
