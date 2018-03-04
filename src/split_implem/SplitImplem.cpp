#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>

int main_split_master(int argc, char **argv)
{
  // todobenoit replace with the main scheduler

  void* handle = dlopen("/home/morelbt/github/raxml-ng/build/src/raxml-ng-mpi.so", RTLD_LAZY);

  if (!handle) {
    cerr << "Cannot open library: " << dlerror() << '\n';
    return 1;
  }
  typedef int (*mainFct)(int,char**,void*);  

  
  mainFct raxml_main = (mainFct) dlsym(handle, "raxml_main");
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
    cerr << "Error while loading symbole raxml_main " << dlsym_error << endl;
    dlclose(handle);
    return 1;
  }
  char ** plop = new char*[3];
  plop[0] = (char*)"raxmlng";
  plop[1] = (char*)"-h";
  MPI_Comm comm = MPI_COMM_SELF;
  int i = raxml_main(2, plop, (void*)&comm);
  return 1;
}

int main_split_slave(int argc, char **argv)
{
  return 0;
}


SplitRanksAllocator::SplitRanksAllocator(int availableRanks,
    const string &outputDir):
  _ranksInUse(0),
  _outputDir(outputDir)
{
}


bool SplitRanksAllocator::ranksAvailable()
{
  return !_slots.empty();
}
 
bool SplitRanksAllocator::allRanksAvailable()
{
  return _ranksInUse == 0;
}
  
InstancePtr SplitRanksAllocator::allocateRanks(int requestedRanks, 
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
  if (result.second < threadsNumber) {
    threadsNumber = result.second;
  }
  _ranksInUse += threadsNumber;
  InstancePtr instance(new SplitInstance(_outputDir,
    startingRank,
    threadsNumber,
    command));
  if (_startedInstances.find(instance->getId()) != _startedInstances.end()) {
    cerr << "Warning: two instances have the same id" << endl;
  }
  _startedInstances[instance->getId()] = instance;
  return instance;
}
  
void SplitRanksAllocator::freeRanks(InstancePtr instance)
{
  _ranksInUse -= instance->getRanksNumber();
  _slots.push(std::pair<int,int>(instance->getStartingRank(), 
        instance->getRanksNumber()));
}

vector<InstancePtr> SplitRanksAllocator::checkFinishedInstances()
{
  vector<string> files;
  string temp = Common::joinPaths(_outputDir, "temp");
  Common::readDirectory(temp, files);
  vector<InstancePtr> finished;
  for (auto file: files) {
    auto instance = _startedInstances.find(file);
    if (instance != _startedInstances.end()) {
      string fullpath = Common::joinPaths(temp, instance->second->getId());
      vector<string> subfiles;
      Common::readDirectory(fullpath, subfiles);
      if (subfiles.size() != instance->second->getRanksNumber()) {
        continue;
      }
      ifstream timerFile(Common::joinPaths(fullpath, subfiles[0]));
      int realElapsedTimeMS = 0;
      timerFile >> realElapsedTimeMS;
      timerFile.close();
      instance->second->setElapsedMs(realElapsedTimeMS);
      Common::removedir(fullpath);
      finished.push_back(instance->second);
    }
  }
  return finished;
}


SplitInstance::SplitInstance(const string &outputDir, 
  int startingRank, 
  int ranksNumber,
  CommandPtr command):
  Instance(command, startingRank, ranksNumber),
  _outputDir(outputDir)
{
}


bool SplitInstance::execute(InstancePtr self)
{
  _beginTime = Common::getTime();
  _finished = false;
  if (_ranksNumber == 0) {
    throw MultiRaxmlException("Error in SplitInstance::execute: invalid number of ranks ", to_string(_ranksNumber));
  }

  string argumentFilename = Common::joinPaths(_outputDir, "orders", getId());
  ofstream argumentFile(argumentFilename);
  argumentFile << getId() << " " << _outputDir << " " << _command->isMpiCommand() << endl;
  for(auto arg: _command->getArgs()) {
    argumentFile << arg << " ";
  }
  argumentFile.close(); 
  
  static string spawnedArg = "--spawned-wrapper";
  char *argv[3];
  argv[0] = (char *)spawnedArg.c_str();
  argv[1] = (char *)argumentFilename.c_str();
  argv[2] = 0;

  return false;
}
  
void SplitInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  drawer.writeSquare(getStartingRank(),
    Common::getElapsedMs(initialTime, getStartTime()),
    getRanksNumber(),
    getElapsedMs());
}
