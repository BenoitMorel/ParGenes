
find_package(MPI REQUIRED)

include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_BUILD_TYPE Debug)

add_executable(multi-raxml main.cpp Command.cpp Common.cpp spawn_implem/SpawnImplem.cpp)
target_link_libraries(multi-raxml ${MPI_LIBRARIES})

if(MPI_COMPILE_FLAGS)
  set_target_properties(multi-raxml PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
  set_target_properties(multi-raxml PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

