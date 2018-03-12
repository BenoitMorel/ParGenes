
#include <mpi.h>
#include <iostream>
#include <string>
#include <thread> 
#include <chrono> 
#include <cstdlib>
#include <fstream>
#include "split_implem/SplitImplem.hpp"

#include "Checkpoint.hpp"
#include "Command.hpp"
#include "Common.hpp"


using namespace std;

namespace MultiRaxml {

  
int _main(int argc, char** argv); 

void main_scheduler(int argc, char **argv)
{
  Common::check(MPI_Init(&argc, &argv));
  
  int rank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  int size = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  if (rank != size - 1) {
    Slave slave;
    slave.main_split_slave(argc, argv);
    return;
  }
  SchedulerArgumentsParser arg(argc, argv);
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  RanksAllocator *allocatorPtr = new SplitRanksAllocator(arg.threadsNumber, 
        arg.outputDir);
  shared_ptr<RanksAllocator> allocator(allocatorPtr);
  CommandsRunner runner(commands, allocator, arg.outputDir);
  runner.run(); 
  Time end = Common::getTime();
  RunStatistics statistics(runner.getHistoric(), begin, end, arg.threadsNumber - 1);
  statistics.printGeneralStatistics();
  statistics.exportSVG(Common::joinPaths(arg.outputDir, "statistics.svg"));
  allocator->terminate();
  Common::check(MPI_Finalize());
  cout << "End of Multiraxml run" << endl;
  exit(0);
}

void main_checkpoint(int argc, char** argv)
{
  if (argc != 3) {
    cout << "Error: wrong syntax with checkpoint" << endl;
    exit(1);
  }
  string outputDir(argv[2]);
  string command = Checkpoint::readCheckpointArgs(outputDir);
  cout << "checkpoint will run :" << endl;
  cout << command << endl;
  system(command.c_str());
}

int _main(int argc, char** argv) 
{
  if (argc < 2) {
    cerr << "Invalid number of parameters. Aborting." << endl;
    return 1;
  }
  string arg(argv[1]);
  if (arg == "--split-scheduler") {
    Checkpoint::writeCheckpointArgs(argc, argv, string(argv[3]));
    main_scheduler(argc, argv);
  } else if (arg == "--checkpoint") {
    main_checkpoint(argc, argv);
  } else {
    cerr << "Unknown argument " << arg << endl;
    return 1;
  }
  return 0;
}

} // namespace MultiRaxml

int main(int argc, char** argv) 
{
  return MultiRaxml::_main(argc, argv);
}

