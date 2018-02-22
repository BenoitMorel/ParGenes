
#include <mpi.h>
#include <iostream>
#include <string>
#include <thread> 
#include <chrono> 
#include <cstdlib>
#include <fstream>
#include "spawn_implem/SpawnImplem.hpp"
#include "mpirun_implem/MpirunImplem.hpp"

#include "Checkpoint.hpp"
#include "Command.hpp"
#include "Common.hpp"


using namespace std;
int _main(int argc, char** argv); 


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

enum SpawnMode {
  SM_MPIRUN,
  SM_MPI_COMM_SPAWN
};

void main_scheduler(int argc, char **argv, SpawnMode mode)
{
  if (mode == SM_MPI_COMM_SPAWN)
    Common::check(MPI_Init(&argc, &argv));
  for (int i = 0; i < argc; ++i) {
    cout << argv[i] << " ";
  }
  cout << endl;
  SchedulerArgumentsParser arg(argc, argv);
  Checkpoint::writeCheckpointArgs(argc, argv, arg.outputDir);
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  RanksAllocator * allocatorPtr = 0;
  if (mode == SM_MPI_COMM_SPAWN) {
    allocatorPtr = new SpawnedRanksAllocator(arg.threadsNumber - 1, 
        arg.outputDir);
  } else {
    allocatorPtr = new MpirunRanksAllocator(arg.threadsNumber - 1, 
        arg.outputDir);
  }
  shared_ptr<RanksAllocator> allocator(allocatorPtr);
  CommandsRunner runner(commands, allocator, arg.threadsNumber - 1, arg.outputDir);
  runner.run(); 
  Time end = Common::getTime();
  RunStatistics statistics(runner.getHistoric(), begin, end, arg.threadsNumber - 1);
  statistics.printGeneralStatistics();
  statistics.exportSVG(arg.outputDir + "/statistics.svg"); // todobenoit not portable
  if (mode == SM_MPI_COMM_SPAWN)
    Common::check(MPI_Finalize());
  cout << "End of Multiraxml run" << endl;
}

void main_checkpoint(int argc, char** argv)
{
  if (argc != 3) {
    cout << "Error: wrong syntax with checkpoint" << endl;
    exit(1);
  }
  string outputDir(argv[2]);
  int newArgc = 0;
  char ** newArgv = 0;
  Checkpoint::readCheckpointArgs(newArgc, newArgv, outputDir);
  _main(newArgc, newArgv);
  for (unsigned int i = 0; i < newArgc; ++i) {
    delete[] newArgv[i];
  }
  delete[] newArgv;
}

/*
 * Several modes:
 * --spawn-scheduler: MPI scheduler that implements the MPI_Comm_spawn approach
 * --mpirun-scheduler: MPI scheduler that implements the mpirun approach
 * --spawned-wrapper: spawned from --spawn_scheduler. Wrapper for the spawned program
 * --mpirun-hostfile: spawned with mpirun. Prints a hostfile.
 * --checkpoint: restart from a checkpoint
 */
int _main(int argc, char** argv) 
{
  if (argc < 2) {
    cerr << "Invalid number of parameters. Aborting." << endl;
    return 1;
  }
  string arg(argv[1]);
  if (arg == "--spawn-scheduler") {
    main_scheduler(argc, argv, SM_MPI_COMM_SPAWN);
  } else if (arg == "--mpirun-scheduler") {
    main_scheduler(argc, argv, SM_MPIRUN);
  } else if (arg == "--spawned-wrapper") {
    main_spawned_wrapper(argc, argv);
  } else if (arg == "--mpirun-hostfile") {
    main_mpirun_hostfile(argc, argv);
  } else if (arg == "--checkpoint") {
    main_checkpoint(argc, argv);
  } else {
    cerr << "Unknown argument " << arg << endl;
    return 1;
  }
  return 0;
}

int main(int argc, char** argv) 
{
  return _main(argc, argv);
}
