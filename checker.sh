
check_file_exists()
  if [ -f $1 ]
  then
    echo "$1 exists... OK!"
  else
    echo "ERROR: $1 does not exist"
    exit 1
  fi

bindir="pargenes/pargenes_binaries"

if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
  check_file_exists "$bindir/raxml-ng-mpi.so"
  check_file_exists "$bindir/modeltest-ng-mpi.so"
fi
check_file_exists "$bindir/raxml-ng"
check_file_exists "$bindir/modeltest-ng"
check_file_exists "$bindir/mpi-scheduler"
echo "End of checks... No error detected!"

