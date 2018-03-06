#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>

namespace MultiRaxml {

const int MASTER_RANK = 0;
const int SIGNAL_SPLIT = 1;
const int SIGNAL_JOB = 2;
const int SIGNAL_END = 3;

const int TAG_END_JOB = 1;

const int MSG_SIZE_END_JOB = 2;



int doWork(const CommandPtr command, 
    MPI_Comm workersComm,
    const string &outputDir) 
{
  std::ofstream out(Common::joinPaths(outputDir, "out.txt"));
  std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
  std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
  cout << "do work " << command->getId() << endl;
  const vector<string> &args  = command->getArgs();
  string lib = args[0] + ".so";
  void *handle = dlopen(lib.c_str(), RTLD_LAZY);
  if (!handle) {
    cerr << "Cannot open shared library " << args[0] << endl;
    return 1;
  }
  mainFct raxmlMain = (mainFct) dlsym(handle, "raxml_main");
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
    cerr << "Cannot load symbole raxml_main " << dlsym_error << endl;
    dlclose(handle);
    return 1;
  }
  int argc = args.size();
  char **argv = new char*[argc];
  for (int i = 0; i < argc; ++i) {
    argv[i] = (char*)args[i].c_str();
  }
  int res = raxmlMain(argc, argv, (void*)&workersComm);
  delete[] argv;
  dlclose(handle);
  MPI_Barrier(workersComm);
  std::cout.rdbuf(coutbuf); 
  return res;
}


int getRank(MPI_Comm comm) {
  int rank = 0;
  MPI_Comm_rank(comm, &rank);
  return rank;
}

int main_split_slave(int argc, char **argv)
{
  SchedulerArgumentsParser arg(argc, argv);
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
  
  MPI_Comm localComm = MPI_COMM_WORLD; 
  int globalRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &globalRank);
  while (true) {
    int signal = 42;
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
      const int maxCommandSize = 200;
      char command[maxCommandSize];
      MPI_Bcast(command, maxCommandSize, MPI_CHAR, MASTER_RANK, localComm);
      MPI_Comm workerComm;
      MPI_Comm_split(localComm, 1, getRank(localComm) - 1, &workerComm);
      Timer timer;
      int jobResult = doWork(commands.getCommand(string(command)), workerComm, arg.outputDir);
      int elapsedMS = timer.getElapsedMs();
      if (!getRank(workerComm)) {
        int endJobMsg[MSG_SIZE_END_JOB];
        endJobMsg[0] = jobResult;
        endJobMsg[1] = elapsedMS;
        MPI_Send(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, MASTER_RANK, TAG_END_JOB, localComm);
      }
    } else if (SIGNAL_END == signal) {
      MPI_Finalize();
      break;
    }
  }
  return 0;
}


SplitRanksAllocator::SplitRanksAllocator(int availableRanks,
    const string &outputDir):
  _ranksInUse(0),
  _outputDir(outputDir)
{
  _slots.push(Slot(1, availableRanks - 1, MPI_COMM_WORLD));
}


bool SplitRanksAllocator::ranksAvailable()
{
  return !_slots.empty();
}
 
bool SplitRanksAllocator::allRanksAvailable()
{
  return _ranksInUse == 0;
}
  
void SplitRanksAllocator::terminate()
{
  while (_slots.size()) {
    int signal = SIGNAL_END;
    MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, _slots.top().comm);
    _slots.pop();
  }
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
  _runningInstances[instance->getId()] = instance;
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
  vector<InstancePtr> finished;
  for (auto startedInstance: _runningInstances) {
    auto instance = static_pointer_cast<SplitInstance>(startedInstance.second);
    MPI_Status status;
    int flag;
    int source = 1;
    MPI_Iprobe(source, TAG_END_JOB, instance->getComm(), &flag, &status); 
    if (!flag) {
      continue;
    }
    int endJobMsg[MSG_SIZE_END_JOB];
    MPI_Recv(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, source, 
        TAG_END_JOB, instance->getComm(), &status);
    instance->setElapsedMs(endJobMsg[1]);
    if (endJobMsg[0]) {
      cerr << "Warning, command " << instance->getId() 
           << " failed with return code " << endJobMsg[0] << endl;
    }
    finished.push_back(instance);
  }
  for (auto instance: finished) {
    _runningInstances.erase(instance->getId());
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
  if (_ranksNumber == 0) {
    throw MultiRaxmlException("Error in SplitInstance::execute: invalid number of ranks ", to_string(_ranksNumber));
  }
  int signal = SIGNAL_JOB;
  MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, getComm());
  MPI_Bcast((char *)self->getId().c_str(), self->getId().size() + 1, MPI_CHAR, MASTER_RANK, getComm()); // todobenoit copy string
  MPI_Comm temp; // will be equal to self
  MPI_Comm_split(getComm(), 0, 0, &temp);
  return true;
}
  
void SplitInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  drawer.writeSquare(getStartingRank(),
    Common::getElapsedMs(initialTime, getStartTime()),
    getRanksNumber(),
    getElapsedMs());
}

} // namespace MultiRaxml

