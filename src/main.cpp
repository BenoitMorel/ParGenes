
#include <mpi.h>
#include <iostream>
#include <string>
#include <thread> 
#include <chrono> 
#include <cstdlib>
#include <fstream>


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

/*
 *  This program is spawned from Command::execute
 */
void spawned(int argc, char** argv) 
{
  bool isMPI = !strcmp(argv[3], "mpi");
  if (!isMPI) 
    MPI_Init(&argc, &argv);
  string str;
  for (unsigned int i = 4; i < argc; ++i) {
    str += string(argv[i]) + " ";
  }
  str += " > /dev/null ";
  system(str.c_str());
  if (!isMPI) 
    MPI_Finalize();
  ofstream out(argv[2]);
  out.close();
}

void spawner(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  ArgumentsParser arg(argc, argv);
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  CommandsRunner runner(commands, arg.threadsNumber - 1, arg.outputDir);
  runner.run(); 
  Time end = Common::getTime();
  CommandsStatistics statistics(commands, begin, end, arg.threadsNumber - 1);
  statistics.printGeneralStatistics();
  statistics.exportSVG(arg.outputDir + "/statistics.svg"); // todobenoit not portable
  MPI_Finalize();
}

int main(int argc, char** argv) 
{
  if (argc < 2) {
    cerr << "Invalid number of parameters. Aborting." << endl;
    return 1;
  }
  if (string(argv[1]) == "--spawned") {
    // spawned by the initial program
    spawned(argc, argv);
  } else {
    // initial program
    spawner(argc, argv);
  }
  return 0;
}
