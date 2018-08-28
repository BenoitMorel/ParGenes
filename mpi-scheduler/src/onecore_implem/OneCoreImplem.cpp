#include "OneCoreImplem.hpp"
#include <cstdlib>
#include <stdio.h>
#include <sys/wait.h>

namespace MPIScheduler {
  
const int SIGNAL_JOB = 2;
const int SIGNAL_TERMINATE = 3;

const int TAG_END_JOB = 1;
const int TAG_START_JOB = 2;
const int TAG_MASTER_SIGNAL = 4;

const int MSG_SIZE_END_JOB = 3;
  

OneCoreSlave::~OneCoreSlave()
{

}

int OneCoreSlave::main_core_slave(int argc, char **argv)
{
  SchedulerArgumentsParser arg(argc, argv);
  _execPath = arg.library;
  _outputDir = arg.outputDir;
  _commands = CommandsContainer(arg.commandsFilename);
  MPI_Comm_size(MPI_COMM_WORLD, &_masterRank);
  _masterRank -= 1;
  while (true) {
    int signal;
    MPI_Status status;
    MPI_Recv(&signal, 1, MPI_INT, _masterRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD, &status);
    if (SIGNAL_JOB == signal) {
      treatJobSlave();         
    } else if (SIGNAL_TERMINATE == signal) {
      MPI_Finalize();
      break;
    }
  }
  return 0;
}

void OneCoreSlave::treatJobSlave()
{
  MPI_Status status;
  const int maxCommandSize = 200;
  char command[maxCommandSize];
  MPI_Recv(command, maxCommandSize, MPI_CHAR, _masterRank, TAG_START_JOB, MPI_COMM_WORLD, &status);
  Timer timer;
  int startingTime = _globalTimer.getElapsedMs();
  int jobResult = doWork(_commands.getCommand(string(command)), _outputDir);
  int elapsedMS = timer.getElapsedMs();
  int endJobMsg[MSG_SIZE_END_JOB];
  endJobMsg[0] = jobResult;
  endJobMsg[1] = startingTime;
  endJobMsg[2] = elapsedMS;
  MPI_Send(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, _masterRank, TAG_END_JOB, MPI_COMM_WORLD);
}



int OneCoreSlave::doWork(const CommandPtr command, 
    const string &outputDir) 
{
  string logsFile = Common::joinPaths(outputDir, "per_job_logs", command->getId() + "_out.txt");
  string runningFile = Common::joinPaths(outputDir, "running_jobs", command->getId());
  ofstream os(runningFile);
  os << logsFile << endl;
  os.close();
  const vector<string> &args  = command->getArgs();
  string systemCommand = _execPath;
  for (auto &arg: args) {
    systemCommand = systemCommand + " " + arg;
  }
  int result = systemCall(systemCommand, logsFile);
  remove(runningFile.c_str());
  return result;
}

OneCoreInstance::OneCoreInstance(const string &outputDir, 
      int rank, 
      CommandPtr command):
  Instance(command, rank, 1, outputDir)
{
}


bool OneCoreInstance::execute(InstancePtr self)
{
  int signal = SIGNAL_JOB;
  MPI_Send(&signal, 1, MPI_INT, self->getStartingRank(), TAG_MASTER_SIGNAL, MPI_COMM_WORLD); 
  MPI_Send((char *)self->getId().c_str(), self->getId().size() + 1, MPI_CHAR, self->getStartingRank(), TAG_START_JOB, MPI_COMM_WORLD);
  return true;
}


OneCoreRanksAllocator::OneCoreRanksAllocator(int availableRanks,
    const string &outputDir):
  _cores(availableRanks - 1),
  _outputDir(outputDir)
{
  Common::makedir(Common::joinPaths(outputDir, "per_job_logs"));
  Common::makedir(Common::joinPaths(outputDir, "running_jobs"));
  for (int i = 0; i < _cores; ++i) {
    _availableCores.push(i);
  }
}

bool OneCoreRanksAllocator::ranksAvailable()
{
  return !_availableCores.empty();
}
 
bool OneCoreRanksAllocator::allRanksAvailable()
{
  return _cores == _availableCores.size();
}
  
void OneCoreRanksAllocator::terminate()
{
  while (_availableCores.size()) {
    int signal = SIGNAL_TERMINATE;
    MPI_Send(&signal, 1, MPI_INT, _availableCores.front(), TAG_MASTER_SIGNAL, MPI_COMM_WORLD);
    _availableCores.pop();
  }
}
  
InstancePtr OneCoreRanksAllocator::allocateRanks(int requestedRanks, 
      CommandPtr command)
{
  int rank = _availableCores.front();
  _availableCores.pop();
  InstancePtr instance(new OneCoreInstance(_outputDir,
    rank,
    command));
  _rankToInstances[rank] = instance;
  return instance;
}

void OneCoreRanksAllocator::freeRanks(InstancePtr instance)
{
  _availableCores.push(instance->getStartingRank());
}

vector<InstancePtr> OneCoreRanksAllocator::checkFinishedInstances()
{
  vector<InstancePtr> finished;
  Timer t;
  while (true) {
    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, TAG_END_JOB, MPI_COMM_WORLD, &flag, &status); 
    if (!flag) {
      break;
    }
    int source = status.MPI_SOURCE;
    int endJobMsg[MSG_SIZE_END_JOB];
    MPI_Recv(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, source,  
        TAG_END_JOB, MPI_COMM_WORLD, &status);
    auto foundInstance = _rankToInstances.find(source);
    if (foundInstance == _rankToInstances.end()) {
      cerr << "Error: did not find the instance started on rank " << source << endl;
      break;
    }
    auto instance = static_pointer_cast<OneCoreInstance>(foundInstance->second);
    instance->setStartingElapsedMS(endJobMsg[1]);
    instance->setElapsedMs(endJobMsg[2]);
    if (endJobMsg[0]) {
      instance->onFailure(endJobMsg[0]);
    }
    finished.push_back(instance);
    //_rankToInstances.erase(source);

  }
  if (finished.size()) {
    cout << "Check finished elapsed time " << t.getElapsedMs()  << " for " << finished.size() << " elements" << endl;
  }
  return finished;
}



}
