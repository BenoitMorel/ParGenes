
#include <mpi.h>
#include <iostream>
#include <string>
#include <thread> 
#include <chrono> 

#include "Command.hpp"
#include "Common.hpp"


using namespace std;


class ArgumentsParser {
public:
  ArgumentsParser(int argc, char** argv):
    commandsFilename(),
    threadsNumber(1)
  {
    if (argc != 4) {
      print_help();
      throw MultiRaxmlException("Error: invalid syntax");
    }
    unsigned int i = 1;
    commandsFilename = string(argv[i++]);
    outputDir = string(argv[i++]);
    threadsNumber = atoi(argv[i++]);
  }
  
  void print_help() 
  {
    cout << "Usage:" << endl;
    cout << "mpirun -np 1 multi-raxml command_file output_dir threads_number" << endl;
  }

  string commandsFilename;
  string outputDir;
  unsigned int threadsNumber;
};

int main(int argc, char** argv) 
{
  MPI_Init(&argc, &argv);
  
  ArgumentsParser arg(argc, argv);
  CommandsContainer commands(arg.commandsFilename);
  CommandsRunner runner(commands, arg.threadsNumber - 1, arg.outputDir);
  runner.run(); 
  MPI_Finalize();
  return 0;
}
