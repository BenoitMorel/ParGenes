
build_mpi_scheduler() {
  cd mpi-scheduler
  mkdir -p build
  cd build
  cmake ..
  make
  cd ../..
}

build_raxml() {
  if [ ! -d "raxml-ng" ]; then
    git clone --recursive https://github.com/amkozlov/raxml-ng.git --branch multi-raxml
  fi
  cd raxml-ng
  mkdir -p build
  cd build
  cmake -DUSE_MPI=ON -DBUILD_AS_LIBRARY=ON ..
  make
  cd ../../
}

build_mpi_scheduler
build_raxml

mpi_scheduler="`pwd`mpi-scheduler/build/mpi-scheduler"
raxml="`pwd`/raxml-ng/bin/raxml-ng-mpi.so"


echo "- mpi_scheduler executable built in ${mpi_scheduler}"
echo "- raxml library built in ${raxml}"

#echo "Run with :"
#echo "mpirun -np cores_number ${mpi_scheduler} ${raxml} command_file output_dir"

