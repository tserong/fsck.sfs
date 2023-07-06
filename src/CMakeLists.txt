cmake_minimum_required(VERSION 3.19)

find_program(CMAKE_C_COMPILER
  NAMES clang gcc
  PATHS /usr/bin
  REQUIRED)

find_program(CMAKE_CXX_COMPILER
  NAMES clang++ g++
  PATHS /usr/bin
  REQUIRED)

set(NAME fsck.s3gw)
project(${NAME} VERSION 0.0.1)

set(BOOST_COMPONENTS program_options)
find_package(Boost 1.79 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

find_package(SQLite3 REQUIRED)
set(SQLITE_COMPILE_FLAGS "-DSQLITE_THREADSAFE=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SQLITE_COMPILE_FLAGS}")

add_compile_options(-Wall -Wextra)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

include_directories(.)
set(sources
  main.cc
  checks.cc
  sqlite.cc
  checks/metadata_schema_version.cc
  checks/orphaned_metadata.cc
  checks/orphaned_objects.cc)
add_executable(${NAME} ${sources})
target_compile_features(${NAME} PUBLIC cxx_std_17)
target_link_libraries(${NAME} ${Boost_LIBRARIES})
target_link_libraries(${NAME} ${SQLite3_LIBRARIES})
