
#include <iostream>
#include <string>

#include "ParallelImplementation.hpp"
#include "Command.hpp"
#include "Common.hpp"


namespace MPIScheduler {

  
int main_scheduler(int argc, char **argv)
{
  // Init
  SchedulerArgumentsParser arg(argc, argv);
  ParallelImplementation implem(arg.implem);
  if (!implem.isValid()) {
    cerr << "Invalid scheduler implementation: " << arg.implem << endl;
    return 1;
  }
  implem.initParallelContext(argc, argv);
  if (implem.slavesToStart()) {
    implem.startSlaves(argc, argv);
  }
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  auto allocator = implem.getRanksAllocator(arg, implem.getRanksNumber());
  for (auto command: commands.getCommands()) {
    allocator->preprocessCommand(command);
  }
  
  // Run 
  CommandsRunner runner(commands, allocator, arg.outputDir, arg.jobFailureFatal);
  if (implem.isOpenMP()) {
    runner.runOpenMP();
  } else {
    runner.run();
  }

  // End
  Time end = Common::getTime();
  RunStatistics statistics(runner.getHistoric(), begin, end, implem.getRanksNumber() - 1);
  statistics.printGeneralStatistics();
  if (runner.getHistoric().size()) {
    statistics.exportSVG(Common::getIncrementalLogFile(arg.outputDir, "statistics", "svg"));
  }
  allocator->terminate();
  implem.closeParallelContext();
  cout << "End of Multiraxml run" << endl;
  return 0;
}

} // namespace MPIScheduler

int main(int argc, char** argv) 
{
  exit(MPIScheduler::main_scheduler(argc, argv));
}

