#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>

const int MASTER_RANK = 0;
const int SIGNAL_SPLIT = 1;
const int SIGNAL_JOB = 2;

int main_split_master(int argc, char **argv)
{
  // todobenoit replace with the main scheduler

  void* handle = dlopen("/home/morelbt/github/raxml-ng/build/src/raxml-ng-mpi.so", RTLD_LAZY);

  if (!handle) {
    cerr << "Cannot open library: " << dlerror() << '\n';
    return 1;
  }

  
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

int doWork(int i, MPI_Comm workersComm) 
{
  Common::sleep(i);
  MPI_Barrier(workersComm);
}


int getRank(MPI_Comm comm) {
  int rank = 0;
  MPI_Comm_rank(comm, &rank);
  return rank;
}

int main_split_slave(int argc, char **argv, MPI_Comm newComm)
{
  MPI_Comm localComm = MPI_COMM_WORLD; 
  int globalRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &globalRank);
  while (true) {
    int signal = 0;
    MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, localComm);
    if (SIGNAL_SPLIT == signal) {
      MPI_Comm temp;
      int splitSize = 0;
      MPI_Bcast(&splitSize, 1, MPI_INT, 0, localComm);
      int localRank = getRank(localComm);
      bool isFirstSplit = localRank <= splitSize;
      MPI_Comm newComm;
      if (isFirstSplit) {
        MPI_Comm_split(localComm, 1, getRank(localComm), &newComm); 
        MPI_Comm_split(localComm, 0, getRank(localComm) + splitSize, &temp);
      } else {
        MPI_Comm_split(localComm, 0, getRank(localComm) + splitSize, &temp); 
        MPI_Comm_split(localComm, 1, getRank(localComm) - splitSize, &newComm);
      }
      localComm = newComm;
    } else if (SIGNAL_JOB == signal) {
    }
  }
  
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

void split(const Slot &parent,
    Slot &son1,
    Slot &son2,
    int son1size)
{
  // send signal
  int signal = SIGNAL_SPLIT;
  MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, parent.comm);
  MPI_Bcast(&son1size, 1, MPI_INT, MASTER_RANK, parent.comm);
  MPI_Comm comm1, comm2;
  MPI_Comm_split(parent.comm, 1, 0, &comm1);
  MPI_Comm_split(parent.comm, 1, 0, &comm2);
  son1 = Slot(parent.startingRank, son1size, comm1);
  son2 = Slot(parent.startingRank + son1size, parent.ranksNumber - son1size, comm2);
}


InstancePtr SplitRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  Slot slot = _slots.top();
  _slots.pop();
  // border case around the first rank
  if (slot.startingRank == 1 && requestedRanks != 1) {
    requestedRanks -= 1; 
  }
  if (slot.ranksNumber > requestedRanks) {
    Slot slot1, slot2;
    split(slot, slot1, slot2, requestedRanks); 
    slot = slot1;
    _slots.push(slot2);
  }
  _ranksInUse += requestedRanks;
  
  InstancePtr instance(new SplitInstance(_outputDir,
    slot.startingRank,
    slot.ranksNumber,
    slot.comm,
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
  auto splitInstance = static_pointer_cast<SplitInstance>(instance);
  _slots.push(Slot(instance->getStartingRank(), 
        instance->getRanksNumber(), splitInstance->getComm()));
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
  MPI_Comm comm,
  CommandPtr command):
  Instance(command, startingRank, ranksNumber),
  _comm(comm),
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

  int work = 3000;
  int masterRank = 0;
  MPI_Bcast(&work, 1, MPI_INT, masterRank, getComm());
  MPI_Comm temp; // will be equal to self
  // create a new communicator without master
  MPI_Comm_split(_comm, 0, 0, &temp);

  /*
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
*/
  return false;
}
  
void SplitInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  drawer.writeSquare(getStartingRank(),
    Common::getElapsedMs(initialTime, getStartTime()),
    getRanksNumber(),
    getElapsedMs());
}
