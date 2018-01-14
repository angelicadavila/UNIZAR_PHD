
macro(SetFlags)

  set(CMAKE_VERBOSE_MAKEFILE ON)

  if (CMAKE_BUILD_TYPE MATCHES "Release")
    set (MODE "RELEASE")
  else ()
    set (MODE "DEBUG")
    set (CMAKE_BUILD_TYPE "Debug")
  endif ()

  # message("${CMAKE_BUILD_TYPE}")

  set(CMAKE_C_COMPILER "clang")
  # set(CMAKE_C_COMPILER "gcc")
  # set(CMAKE_C_COMPILER "icc")
  set(CMAKE_CXX_COMPILER "clang++")
  # set(CMAKE_CXX_COMPILER "g++")

  set (CC $ENV{CC})
  if (CC)
    message("Using CC compiler: ${CC}")
    set(CMAKE_C_COMPILER "${CC}")
  endif()
  set (CXX $ENV{CXX})
  if (CXX)
    message("Using CXX compiler: ${CXX}")
    set(CMAKE_CXX_COMPILER "${CXX}")
  endif()

  set(CMAKE_C_FLAGS_DEBUG "")
  set(CMAKE_CXX_FLAGS_DEBUG "")
  set(CMAKE_C_FLAGS_RELEASE "")
  set(CMAKE_CXX_FLAGS_RELEASE "")


  # TODO add -Werror
  # set(CMAKE_COMMON_FLAGS "-std=c++17 -Wall -pedantic -Wextra -fmax-errors=1 -Wno-unused-parameter -Wno-unknown-pragmas")
  set(CMAKE_COMMON_FLAGS " -Wall -Wextra ")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 ")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wthread-safety")
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")

  if (CMAKE_C_COMPILER MATCHES "clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ferror-limit=1")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmax-errors=1")
  endif()
  if (CMAKE_CXX_COMPILER MATCHES "clang\\+\\+")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ferror-limit=1")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=1")
  endif()

  IF (PROFILE MATCHES "y")
    message ("profiling: yes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg ")
  ELSE()
    message ("profiling: no")
  ENDIF (PROFILE MATCHES "y")


  if (MODE MATCHES "RELEASE")
    # TODO: this is stripping, you have to do use the strip tool with the release binary
    # strip binary.elf
    # set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    # set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wl,-s")
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wl,-s")
    set(CMAKE_DEBUG_FLAGS "-pedantic")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CMAKE_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_DEBUG_FLAGS}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CMAKE_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_DEBUG_FLAGS}")
    set(OPTIMIZE "y")
    # set(DEBUG "n")
    set (DEBUG $ENV{DEBUG})
  else ()
    set(CMAKE_DEBUG_FLAGS "-Wno-unused-parameter -Wno-unknown-pragmas -Wno-ignored-attributes -Wno-unused-variable")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_DEBUG_FLAGS}")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_DEBUG_FLAGS}")
    add_definitions ("-DBUILD_TYPE_IS_DEBUG=true")
    # set(OPTIMIZE "n")
    set (OPTIMIZE $ENV{OPTIMIZE})
    set(DEBUG "y")
  endif ()

  IF (OPTIMIZE MATCHES "y")
    message ("optimize (-O3): yes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  ELSE()
    message ("optimize (-O3): no")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
  ENDIF (OPTIMIZE MATCHES "y")

  IF (DEBUG MATCHES "y")
    message ("debug (g): yes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  ELSE()
    message ("debug (g): no")
  ENDIF (DEBUG MATCHES "y")

  set (CMAKE_EXPORT_COMPILE_COMMANDS "1")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_COMMON_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_COMMON_FLAGS}")

  message ("cmake build type: ${CMAKE_BUILD_TYPE}")
  message ("node: ${NODE}")

endmacro()
