cmake_minimum_required(VERSION 3.0)
project(MULTI_RAXML)


set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 11)

if (NOT DISABLE_MPI)
  include(FindMPI)
  if(MPI_FOUND)
    set(WITH_MPI TRUE)
    add_definitions(-DWITH_MPI)
    message(STATUS "WITH MPI ${OpenMP_EXE_LINKER_FLAGS}")
  endif()
endif()

if (NOT DISABLE_OPENMP)
  include(FindOpenMP)
  if(OPENMP_FOUND)
    set(WITH_OPENMP TRUE)
    add_definitions(-DWITH_OPENMP)
    message(STATUS "WITH OPENMP ${OpenMP_EXE_LINKER_FLAGS}")
  endif()
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

set(CMAKE_CXX_FLAGS "-Weverything ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-Weverything ${CMAKE_C_FLAGS}")

