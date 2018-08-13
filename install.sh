
build_mpi_scheduler() {
  echo "*********************************"
  echo "** Installing mpi_scheduler... **"
  echo "*********************************"
  cd mpi-scheduler
  mkdir -p build
  cd build
  cmake .. || exit 1
  make -j 4 || exit 1
  cd ..
  cd ../..
}

build_raxml_lib() {
  echo "************************************"
  echo "** Installing raxml-ng library ...**"
  echo "************************************"
  cd raxml-ng
  mkdir -p build
  cd build
  cmake -DUSE_MPI=ON -DBUILD_AS_LIBRARY=ON .. || exit 1
  make -j 4 || exit 1
  cd ../../
}

build_raxml_exec() {
  echo "***************************************"
  echo "** Installing raxml-ng executable ...**"
  echo "***************************************"
  cd raxml-ng
  mkdir -p build
  cd build
  cmake -DUSE_MPI=OFF -DBUILD_AS_LIBRARY=OFF .. || exit 1
  cmake .. || exit 1
  make -j 4 || exit 1
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
  make -j 4 || exit 1
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
  make -j 4 || exit 1
  cd ../../
}


#build_mpi_scheduler
#build_raxml_lib
#build_raxml_exec
build_modeltest_lib
build_modeltest_exec


