
find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_BUILD_TYPE Debug)

add_executable(wrapper wrapper.cpp)
target_link_libraries(wrapper ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
  set_target_properties(wrapper PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
  set_target_properties(wrapper PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

