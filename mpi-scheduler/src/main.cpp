
#include <iostream>
#include <string>
//#include <thread> 
#include <chrono> 
#include <cstdlib>
#include <fstream>
#include <assert.h>

#include "Checkpoint.hpp"
#include "Command.hpp"
#include "Common.hpp"

#ifdef WITH_MPI
#include "split_implem/SplitImplem.hpp"
#include "onecore_implem/OneCoreImplem.hpp"
#include <mpi.h>
#endif
#ifdef WITH_OPENMP
#include "openmp_implem/OpenMPImpl.hpp"
#endif

using namespace std;

namespace MPIScheduler {

  
int _main(int argc, char** argv); 



bool isMPIImplem(const string &implem) {
  return (implem == "--split-scheduler" || implem == "--onecore-scheduler");
}

bool checkImplemCompatible(const string &implem) {

}

RanksAllocator *getRanksAllocator(const string &implem,
                                  SchedulerArgumentsParser &arg,
                                  int ranksNumber) {
#ifdef WITH_MPI
  if (implem == "--split-scheduler") {
    return new SplitRanksAllocator(ranksNumber,
        arg.outputDir);
  } else if (implem == "--onecore-scheduler") {
    return new OneCoreRanksAllocator(ranksNumber,
        arg.outputDir);
  }
#endif
  if (implem == "--openmp-scheduler") {
    return new OpenMPRanksAllocator(arg.outputDir, arg.library);
  }
  assert(0);
  return 0;
}

void init_mpi(int argc, char **argv, int *rank, int *ranksNumber)
{
#ifdef WITH_MPI
  Common::check(MPI_Init(&argc, &argv));
  MPI_Comm_rank(MPI_COMM_WORLD, rank);
  MPI_Comm_size(MPI_COMM_WORLD, ranksNumber);
#else
  assert(0);
#endif
}

void end_mpi()
{
#ifdef WITH_MPI
  MPI_Finalize();
#endif
}

void run_slave_main(const string &implem, int argc, char **argv)
{
#ifdef WITH_MPI
  if (implem == "--split-scheduler") {
    SplitSlave slave;
    slave.main_split_slave(argc, argv);
  } else if (implem == "--onecore-scheduler") {
    OneCoreSlave slave;
    slave.main_core_slave(argc, argv);
  }
#endif
}

void main_scheduler(int argc, char **argv)
{
  SchedulerArgumentsParser arg(argc, argv);
  string implem = arg.implem;
  int rank = 0;
  int ranksNumber = 0;

  if (isMPIImplem(implem)) {
    init_mpi(argc, argv, &rank, &ranksNumber);
    if (rank != ranksNumber - 1) {
      run_slave_main(implem, argc, argv);
      return;
    }
  }

  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  RanksAllocator *allocatorPtr = getRanksAllocator(implem, arg,ranksNumber);
  shared_ptr<RanksAllocator> allocator(allocatorPtr);
  for (auto command: commands.getCommands()) {
    allocator->preprocessCommand(command);
  }
  CommandsRunner runner(commands, allocator, arg.outputDir);
  if (implem == "--openmp-scheduler") {
    runner.runOpenMP();
  } else {
    runner.run();
  }
  Time end = Common::getTime();
  RunStatistics statistics(runner.getHistoric(), begin, end, ranksNumber - 1);
  statistics.printGeneralStatistics();

  if (runner.getHistoric().size()) {
    statistics.exportSVG(Common::getIncrementalLogFile(arg.outputDir, "statistics", "svg"));
  }
  allocator->terminate();

  if (isMPIImplem(implem))
    end_mpi();
  cout << "End of Multiraxml run" << endl;
  exit(0);
}

} // namespace MPIScheduler

int main(int argc, char** argv) 
{
  MPIScheduler::main_scheduler(argc, argv);
  return 0;
}

