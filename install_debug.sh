
build_mpi_scheduler() {
  cd mpi-scheduler
  mkdir -p build
  cd build
  cmake ..
  make
  cd ../..
}

build_raxml() {
  cd raxml-ng
  mkdir -p builddebug
  cd builddebug
  cmake ..
  make
  cd ../../
}

build_modeltest() {
  cd modeltest
  mkdir -p builddebug
  cd builddebug
  cmake ..
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

