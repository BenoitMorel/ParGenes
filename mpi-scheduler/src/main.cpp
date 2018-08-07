
#include <iostream>
#include <string>
//#include <thread> 
#include <chrono> 
#include <cstdlib>
#include <fstream>

#include "Checkpoint.hpp"
#include "Command.hpp"
#include "Common.hpp"

#ifdef WITH_MPI
#include "split_implem/SplitImplem.hpp"
#include "onecore_implem/OneCoreImplem.hpp"
#include <mpi.h>
#endif

using namespace std;

namespace MPIScheduler {

  
int _main(int argc, char** argv); 

void main_scheduler(int argc, char **argv)
{
  SchedulerArgumentsParser arg(argc, argv);
  string implem = argv[1]; 
  int rank = 0;
  int ranksNumber = 0;
  
#ifdef WITH_MPI
  Common::check(MPI_Init(&argc, &argv));
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranksNumber);
  if (rank != ranksNumber - 1) {
    if (implem == "--split-scheduler") {
      SplitSlave slave;
      slave.main_split_slave(argc, argv);
    } else if (implem == "--onecore-scheduler") {
      OneCoreSlave slave;
      slave.main_core_slave(argc, argv);
    }
    return;
  }
#endif
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  RanksAllocator *allocatorPtr = 0;
#ifdef WITH_MPI
  if (implem == "--split-scheduler") {
    allocatorPtr = new SplitRanksAllocator(ranksNumber, 
        arg.outputDir);
  } else if (implem == "--onecore-scheduler") {
    allocatorPtr = new OneCoreRanksAllocator(ranksNumber, 
        arg.outputDir);
  }
#endif
  shared_ptr<RanksAllocator> allocator(allocatorPtr);
  for (auto command: commands.getCommands()) {
    allocator->preprocessCommand(command);
  }
  CommandsRunner runner(commands, allocator, arg.outputDir);
  runner.run(); 
  Time end = Common::getTime();
  RunStatistics statistics(runner.getHistoric(), begin, end, ranksNumber - 1);
  statistics.printGeneralStatistics();

  if (runner.getHistoric().size()) {
    statistics.exportSVG(Common::getIncrementalLogFile(arg.outputDir, "statistics", "svg"));
  }
  allocator->terminate();
#ifdef WITH_MPI
  Common::check(MPI_Finalize());
#endif
  cout << "End of Multiraxml run" << endl;
  exit(0);
}

} // namespace MPIScheduler

int main(int argc, char** argv) 
{
  MPIScheduler::main_scheduler(argc, argv);
  return 0;
}

