#include "OpenMPImpl.hpp"
#include <omp.h>
#include <cstdlib>
#include <stdio.h>
#include <sys/wait.h>
#include "../Common.hpp"
#include "../Command.hpp"

namespace MPIScheduler {

int getOpenMPThreads() {
  return omp_get_max_threads();
}

int doWork(const CommandPtr command,
    InstancePtr instance,
    const string &execPath,
    const string &outputDir) 
{
  int result = 0;
  Timer timer;
  string logsFile = Common::joinPaths(outputDir, "per_job_logs", command->getId() + "_out.txt");
  string runningFile = Common::joinPaths(outputDir, "running_jobs", command->getId());
  ofstream os(runningFile);
  os << logsFile << endl;
  os.close();
  const vector<string> &args  = command->getArgs();
  string systemCommand = execPath;
  for (auto &arg: args) {
    systemCommand = systemCommand + " " + arg;
  }
  result = systemCall(systemCommand, logsFile, true);
  remove(runningFile.c_str());
  instance->setElapsedMs(timer.getElapsedMs());
  return result;
}

OpenMPRanksAllocator::OpenMPRanksAllocator(const string &outputDir, const string &execPath): 
    _outputDir(outputDir),
    _execPath(execPath)
{
  Common::makedir(Common::joinPaths(outputDir, "per_job_logs"));
  Common::makedir(Common::joinPaths(outputDir, "running_jobs"));
}

InstancePtr OpenMPRanksAllocator::allocateRanks(int requestedRanks, 
      CommandPtr command)
{
  int tid = omp_get_thread_num();
  auto instance = InstancePtr(new OpenMPInstance(_outputDir, _execPath, tid, command));
  instance->setStartingElapsedMS(_globalTimer.getElapsedMs());
  return instance;
}

OpenMPInstance::OpenMPInstance(const string &outputDir, 
      const string &execPath,
      int rank, 
      CommandPtr command):
  Instance(command, rank, 1, outputDir),
  _execPath(execPath)
{
}

bool OpenMPInstance::execute(InstancePtr self)
{
  int res = 0;
  res = doWork(_command, self, _execPath, _outputDir);
  if (res) {
    cerr << "Warning, command " << _command->getId() << " failed with code " << res << endl;
    return false;
  }
  return true;
}

}
