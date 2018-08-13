mkdir -p build_no_mpi
cd build_no_mpi
cmake .. -DDISABLE_MPI=TRUE
make -j 4
cd ..


mkdir -p build_no_openmp
cd build_no_openmp
cmake .. -DDISABLE_OPENMP=TRUE
make -j 4
cd ..


mkdir -p build_all
cd build_all
cmake ..
make -j 4
cd ..




