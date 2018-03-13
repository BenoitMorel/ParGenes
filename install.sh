
build_multiraxml() {
  mkdir -p build
  cd build
  cmake ..
  make
  cd ..
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

build_multiraxml
build_raxml

multi_raxml="`pwd`/build/multi-raxml"
raxml="`pwd`/raxml-ng/bin/raxml-ng-mpi.so"


echo "- multi_raxml executable built in ${multi_raxml}"
echo "- raxml library built in ${raxml}"

echo "Run with :"
echo "mpirun -np cores_number ${multi_raxml} ${raxml} command_file output_dir"

