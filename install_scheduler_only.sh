
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

check_recursive
./uninstall.sh

cores=4
if [ "$#" -eq 1 ]; then
  cores=$1
  echo "yo"
fi

echo "Installing with $cores cores"

build_mpi_scheduler $cores
build_astral

