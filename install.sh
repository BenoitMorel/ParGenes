
build_mpi_scheduler() {
  echo "*********************************"
  echo "** Installing mpi_scheduler... **"
  echo "*********************************"
  cd mpi-scheduler
  mkdir -p build
  cd build
  cmake ..
  make
  cd ../..
}

build_raxml() {
  echo "*********************************"
  echo "** Installing raxml-ng...      **"
  echo "*********************************"
  cd raxml-ng
  mkdir -p build
  cd build
  cmake -DUSE_MPI=ON -DBUILD_AS_LIBRARY=ON ..
  make
  cd ../../
}

build_modeltest() {
  echo "*********************************"
  echo "** Installing model-test-ng... **"
  echo "*********************************"
  cd modeltest
  mkdir -p build
  cd build
  cmake -DUSE_MPI=ON -DBUILD_AS_LIBRARY=ON ..
  make
  cd ../../
}

build_mpi_scheduler
build_raxml
build_modeltest

mpi_scheduler="`pwd`mpi-scheduler/build/mpi-scheduler"
raxml="`pwd`/raxml-ng/bin/raxml-ng-mpi.so"
modeltest="`pwd`/modeltest/bin/modeltest-ng"


echo "- mpi_scheduler executable built in ${mpi_scheduler}"
echo "- raxml library built in ${raxml}"
echo "- modeltest built in ${modeltest}"

