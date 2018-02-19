#include "SpawnImplem.h"
#include <mpi.h>
#include <iostream>

SpawnedRanksAllocator::SpawnedRanksAllocator(int availableRanks,
    const string &outputDir):
  _ranksInUse(0),
  _outputDir(outputDir)
{
  _slots.push(std::pair<int,int>(1, availableRanks));
}


bool SpawnedRanksAllocator::ranksAvailable()
{
  return !_slots.empty();
}
 
bool SpawnedRanksAllocator::allRanksAvailable()
{
  return _ranksInUse == 0;
}
  
InstancePtr SpawnedRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  std::pair<int, int> result = _slots.top();
  _slots.pop();
  int startingRank = result.first;
  int threadsNumber = requestedRanks; 
  // border case around the first rank
  if (result.first == 1 && requestedRanks != 1) {
    threadsNumber -= 1; 
  }
  if (result.second > threadsNumber) {
    result.first += threadsNumber;
    result.second -= threadsNumber;
    _slots.push(result);
  }
  _ranksInUse += threadsNumber;
  InstancePtr instance(new SpawnInstance(_outputDir,
    startingRank,
    threadsNumber,
    command));
  if (_startedInstances.find(instance->getId()) != _startedInstances.end()) {
    cerr << "Warning: two instances have the same id" << endl;
  }
  _startedInstances[instance->getId()] = instance;
  return instance;
}
  
void SpawnedRanksAllocator::freeRanks(InstancePtr instance)
{
  _ranksInUse -= instance->getRanksNumber();
  _slots.push(std::pair<int,int>(instance->getStartingRank(), 
        instance->getRanksNumber()));
}

vector<InstancePtr> SpawnedRanksAllocator::checkFinishedInstances()
{
  vector<string> files;
  Common::readDirectory(_outputDir, files);
  vector<InstancePtr> finished;
  for (auto file: files) {
    auto instance = _startedInstances.find(file);
    if (instance != _startedInstances.end()) {
      string fullpath = _outputDir + "/" + instance->second->getId(); // todobenoit not portable
      Common::removefile(fullpath);
      if (!instance->second->didFinish()) { // sometime the file is written several times
        finished.push_back(instance->second);
      }
    }
  }
  return finished;
}


SpawnInstance::SpawnInstance(const string &outputDir, 
  int startingRank, 
  int ranksNumber,
  CommandPtr command):
  Instance(command, startingRank, ranksNumber),
  _outputDir(outputDir)
{
}


void SpawnInstance::execute()
{
  _finished = false;
  if (_ranksNumber == 0) {
    throw MultiRaxmlException("Error in SpawnInstance::execute: invalid number of ranks ", to_string(_ranksNumber));
  }
  const vector<string> &args = _command->getArgs();
  char **argv = new char*[args.size() + 3];
  string infoFile = _outputDir + "/" + getId(); // todobenoit not portable
  string spawnedArg = "--spawned-wrapper";
  string isMPIStr = _command->isMpiCommand() ? "mpi" : "nompi";
  argv[0] = (char *)spawnedArg.c_str();
  argv[1] = (char *)infoFile.c_str();
  argv[2] = (char *)isMPIStr.c_str();
  unsigned int offset = 3;
  for(unsigned int i = 0; i < args.size(); ++i)
    argv[i + offset] = (char*)args[i].c_str();
  argv[args.size() + offset] = 0;

  Timer t;

  string wrapperExec = Common::getSelfpath();

  MPI_Comm intercomm;
  MPI_Comm_spawn((char*)wrapperExec.c_str(), argv, getRanksNumber(),  
          MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomm,  
          MPI_ERRCODES_IGNORE);
  cout << "submit time " << t.getElapsedMs() << endl;
  delete[] argv;
  _beginTime = Common::getTime();
}
  
void SpawnInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  drawer.writeSquare(getStartingRank(),
    Common::getElapsedMs(initialTime, getStartTime()),
    getRanksNumber(),
    getElapsedMs());
}
