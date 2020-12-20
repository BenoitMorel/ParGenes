#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
bindir="$DIR/pargenes/pargenes_binaries"
echo "Running from $DIR"

install() {
  echo "cp -r $1 $bindir"
  cp -r $1 $bindir
}

build_mpi_scheduler() {
  echo "*********************************"
  echo "** Installing mpi_scheduler... **"
  echo "*********************************"
  cd MPIScheduler
  mkdir -p build
  cd build
  cmake .. || exit 1
  make -j $1 || exit 1
  cd ../..
}

build_raxml_lib() {
  echo "************************************"
  echo "** Installing raxml-ng library ...**"
  echo "************************************"
  cd raxml-ng
  mkdir -p build
  cd build
  cmake -DUSE_TERRAPHAST=OFF -DUSE_MPI=ON -DBUILD_AS_LIBRARY=ON .. || exit 1
  make -j $1 || exit 1
  cd ../../
}

build_raxml_exec() {
  echo "***************************************"
  echo "** Installing raxml-ng executable ...**"
  echo "***************************************"
  cd raxml-ng
  mkdir -p build
  cd build
  cmake -DUSE_TERRAPHAST=OFF -DUSE_MPI=OFF -DBUILD_AS_LIBRARY=OFF .. || exit 1
  cmake .. || exit 1
  make -j $1 || exit 1
  cd ../../
}

build_modeltest_lib() {
  echo "******************************************"
  echo "** Installing model-test-ng library ... **"
  echo "******************************************"
  cd modeltest
  mkdir -p build
  cd build
  cmake -DUSE_MPI=ON -DBUILD_AS_LIBRARY=ON .. || exit 1
  make -j $1 || exit 1
  cd ../../
}

build_modeltest_exec() {
  echo "******************************************"
  echo "** Installing model-test-ng executable ... **"
  echo "******************************************"
  cd modeltest
  mkdir -p build
  cd build
  cmake -DUSE_MPI=OFF -DBUILD_AS_LIBRARY=OFF -DUSE_LIBPLL_CMAKE=ON .. || exit 1
  make -j $1 || exit 1
  cd ../../
}

build_astral() {
  echo "******************************************"
  echo "** Installing ASTRAL...                 **"
  echo "******************************************"
  cd ASTRAL
  unzip -o *.zip
  cd Astral
  mv astral.5.6.3.jar astral.jar
  cd ../../
}

print_no_recursive_warning() {
  echo "[Error] Could not find the file $1"
  echo "Please check that you downloaded ParGenes with git clone --recursive, as explained in the README."
}

check_recursive() {
  f=raxml-ng/CMakeLists.txt
  if [ ! -f $f ]; then
    print_no_recursive_warning $f
    exit
  fi
}

finish_install() {
  echo ""
  echo "Copying all binaries into $bindir"
  mkdir -p $bindir
  install "MPIScheduler/build/mpi-scheduler"
  install "raxml-ng/bin/raxml-ng"
  install "modeltest/bin/modeltest-ng" 
  if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    install "raxml-ng/bin/raxml-ng-mpi.so"
    install "modeltest/build/src/modeltest-ng-mpi.so"
  fi
  install "ASTRAL/Astral/astral.jar"
  install "ASTRAL/Astral/lib"
  echo ""
}

check_recursive
./uninstall.sh

cores=4
if [ "$#" -eq 1 ]; then
  cores=$1
fi

echo "Installing with $cores cores"
build_mpi_scheduler $cores
build_raxml_lib $cores
build_raxml_exec $cores
build_modeltest_lib $cores
build_modeltest_exec $cores
build_astral
finish_install

echo "Running unit tests..."
python tests/run_tests.py

