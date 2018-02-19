
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


class SchedulerArgumentsParser {
public:
  SchedulerArgumentsParser(int argc, char** argv):
    commandsFilename(),
    threadsNumber(1)
  {
    if (argc != 5) {
      print_help();
      throw MultiRaxmlException("Error: invalid syntax");
    }
    unsigned int i = 2;
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
void main_spawned_wrapper(int argc, char** argv) 
{
  string id = argv[2];
  bool isMPI = !strcmp(argv[3], "mpi");
  if (!isMPI) 
    MPI_Init(&argc, &argv);
  string command;
  for (unsigned int i = 4; i < argc; ++i) {
    command += string(argv[i]) + " ";
  }
  command += " > " +  id + ".spawned.out 2>&1 ";
  try {
    system(command.c_str());
  } catch(...) {
    ofstream out(id + ".failure");
    out << ("Command " + id + " failed");
  }
  
  if (!isMPI) 
    MPI_Finalize();
  ofstream out(id);
  out.close();
}

void main_spawn_scheduler(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  SchedulerArgumentsParser arg(argc, argv);
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  CommandsRunner runner(commands, arg.threadsNumber - 1, arg.outputDir);
  runner.run(); 
  Time end = Common::getTime();
  RunStatistics statistics(runner.getHistoric(), begin, end, arg.threadsNumber - 1);
  statistics.printGeneralStatistics();
  statistics.exportSVG(arg.outputDir + "/statistics.svg"); // todobenoit not portable
  MPI_Finalize();
}

void main_mpirun_scheduler(int argc, char** argv)
{
  cout << "not implemented" << endl;
}
  

/*
 * Several mains:
 * --spawn-scheduler: MPI scheduler that implements the MPI_Comm_spawn approach
 * --mpirun-scheduler: MPI scheduler that implements the mpirun approach
 * --spawned-wrapper: spanwed from --spawn_scheduler. Wrapper for the spawned program
 */
int main(int argc, char** argv) 
{
  if (argc < 2) {
    cerr << "Invalid number of parameters. Aborting." << endl;
    return 1;
  }
  string arg(argv[1]);
  if (arg == "--spawn-scheduler") {
    main_spawn_scheduler(argc, argv);
  } else if (arg == "--mpirun-scheduler") {
    main_mpirun_scheduler(argc, argv);
  } else if (arg == "--spawned-wrapper") {
    main_spawned_wrapper(argc, argv);
  } else {
    cerr << "Unknown argument " << arg << endl;
    return 1;
  }
  return 0;
}
