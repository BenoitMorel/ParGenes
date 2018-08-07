

set(CMAKE_BUILD_TYPE Debug)


set(MPI_SCHEDULER_SOURCES main.cpp
  Checkpoint.cpp
  Command.cpp 
  Common.cpp 
  SVGDrawer.cpp
)
set (MPI_SCHEDULER_LIBRARIES dl gcc_s gcc)

if (WITH_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_INCLUDE_PATH})
  set(MPI_SCHEDULER_SOURCES ${MPI_SCHEDULER_SOURCES}
    split_implem/SplitImplem.cpp
    onecore_implem/OneCoreImplem.cpp
  )
  set(MPI_SCHEDULER_LIBRARIES ${MPI_LIBRARIES} ${MPI_SCHEDULER_LIBRARIES})
endif()




add_executable(mpi-scheduler ${MPI_SCHEDULER_SOURCES}) 



target_link_libraries(mpi-scheduler ${MPI_SCHEDULER_LIBRARIES})

if (WITH_MPI)
  if(MPI_COMPILE_FLAGS)
    set_target_properties(mpi-scheduler PROPERTIES
      COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
  endif()

  if(MPI_LINK_FLAGS)
    set_target_properties(mpi-scheduler PROPERTIES
      LINK_FLAGS "${MPI_LINK_FLAGS}")
  endif()
endif()
