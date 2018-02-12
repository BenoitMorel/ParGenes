
#include <mpi.h>
#include <iostream>
#include <string>
#include <thread> 
#include <chrono> 

#include "Command.hpp"


using namespace std;

void print_help() 
{
  cout << "Usage:" << endl;
  cout << "mpirun -np 1 multi-raxml command_file" << endl;
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  if (argc != 2) {
    cerr << "Invalid syntax" << std::endl;
    print_help();
    return 0;
  }

  string commandsFilename(argv[1]);

  CommandManager commands(commandsFilename);

  MPI_Finalize();
  return 0;
}
