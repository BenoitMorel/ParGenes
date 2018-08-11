#include "OpenMPImpl.hpp"
#include <omp.h>
#include <cstdlib>
#include <stdio.h>
#include <sys/wait.h>
#include "../Common.hpp"
#include "../Command.hpp"

namespace MPIScheduler {


int doWork(const CommandPtr command, 
    const string &execPath,
    const string &outputDir) 
{
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
  int result = Common::systemCall(systemCommand, logsFile);
  remove(runningFile.c_str());
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
  return InstancePtr(new OpenMPInstance(_outputDir, _execPath, tid, command));
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
  int res = doWork(_command, _execPath, _outputDir);
  if (!res) {
    cerr << "Warning, command " << _command->getId() << " failed with code " << res << endl;
  }
  return true;
}

}
