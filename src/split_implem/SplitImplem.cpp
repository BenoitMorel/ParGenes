#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>

namespace MultiRaxml {

const int SIGNAL_SPLIT = 1;
const int SIGNAL_JOB = 2;
const int SIGNAL_TERMINATE = 3;

const int TAG_END_JOB = 1;
const int TAG_START_JOB = 2;
const int TAG_SPLIT = 3;
const int TAG_MASTER_SIGNAL = 4;

const int MSG_SIZE_END_JOB = 3;


int getRank(MPI_Comm comm) {
  int rank = 0;
  MPI_Comm_rank(comm, &rank);
  return rank;
}
int getSize(MPI_Comm comm) {
  int size = 0;
  MPI_Comm_size(comm, &size);
  return size;
}


int getMasterRank()
{
  return getSize(MPI_COMM_WORLD) - 1;
}

int getLocalMasterRank(MPI_Comm comm)
{
  return 0;
}

int doWork(const CommandPtr command, 
    MPI_Comm workersComm,
    const string &outputDir) 
{
  std::ofstream out(Common::joinPaths(outputDir, "per_job_logs", command->getId() + "out.txt"));
  std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
  std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
  const vector<string> &args  = command->getArgs();
  string lib = args[0] + ".so";
  void *handle = dlopen(lib.c_str(), RTLD_LAZY);
  if (!handle) {
    cerr << "Cannot open shared library " << lib << endl;
    return 1;
  }
  mainFct raxmlMain = (mainFct) dlsym(handle, "exportable_main");
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
    cerr << "Cannot load symbole exportable_main " << dlsym_error << endl;
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


void splitSlave(MPI_Comm &localComm) 
{
  int localRank = getRank(localComm);
  MPI_Status status;
  int splitSize;
  if (getLocalMasterRank(localComm) == getRank(localComm)) {
    MPI_Recv(&splitSize, 1, MPI_INT, getMasterRank(), TAG_SPLIT, MPI_COMM_WORLD, &status);
    int signal = SIGNAL_SPLIT;
    MPI_Bcast(&signal, 1, MPI_INT, getLocalMasterRank(localComm), localComm);
  }
  MPI_Bcast(&splitSize, 1, MPI_INT, getLocalMasterRank(localComm), localComm);
  int color = localRank / splitSize;
  int newRank = localRank % splitSize;
  MPI_Comm newComm;
  MPI_Comm_split(localComm, color , newRank, &newComm);
  localComm = newComm;
}

void treatJobSlave(MPI_Comm localComm, 
    CommandsContainer &commands,
    const string &outputDir,
    Timer &globalTimer)
{
  bool alone = getSize(localComm) == 1;
  MPI_Status status;
  const int maxCommandSize = 200;
  char command[maxCommandSize];
  if (getLocalMasterRank(localComm) == getRank(localComm)) {
    MPI_Recv(command, maxCommandSize, MPI_CHAR, getMasterRank(), TAG_START_JOB, MPI_COMM_WORLD, &status);
    if (!alone) {
      int signal = SIGNAL_JOB;
      MPI_Bcast(&signal, 1, MPI_INT, getLocalMasterRank(localComm), localComm);
    }
  }

  if (!alone) {
    MPI_Bcast(command, maxCommandSize, MPI_CHAR, getLocalMasterRank(localComm), localComm);
  }
  Timer timer;
  int startingTime = globalTimer.getElapsedMs();
  int jobResult = doWork(commands.getCommand(string(command)), localComm, outputDir);
  int elapsedMS = timer.getElapsedMs();
  if (getLocalMasterRank(localComm) == getRank(localComm)) {
    int endJobMsg[MSG_SIZE_END_JOB];
    endJobMsg[0] = jobResult;
    endJobMsg[1] = startingTime;
    endJobMsg[2] = elapsedMS;
    MPI_Send(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, getMasterRank(), TAG_END_JOB, MPI_COMM_WORLD);
  }
}

void terminateSlave(MPI_Comm localComm)
{
  if (getLocalMasterRank(localComm) == getRank(localComm)) {
    int signal = SIGNAL_TERMINATE;
    MPI_Bcast(&signal, 1, MPI_INT, getLocalMasterRank(localComm), localComm);
  }
  MPI_Finalize();
}

int main_split_slave(int argc, char **argv)
{
  Timer globalTimer;
  SchedulerArgumentsParser arg(argc, argv);
  Time begin = Common::getTime();
  CommandsContainer commands(arg.commandsFilename);
   
  MPI_Comm localComm; 
  // initial split to get rid of the master process
  MPI_Comm_split(MPI_COMM_WORLD, 1, getRank(MPI_COMM_WORLD), &localComm);
  int globalRank = getRank(MPI_COMM_WORLD);
  while (true) {
    if (!getRank(localComm)) {
      int signal;
      MPI_Status status;
      MPI_Recv(&signal, 1, MPI_INT, getMasterRank(), TAG_MASTER_SIGNAL, MPI_COMM_WORLD, &status);
      if (SIGNAL_SPLIT == signal) {
        splitSlave(localComm);
      } else if (SIGNAL_JOB == signal) {
        treatJobSlave(localComm, commands, arg.outputDir, globalTimer);         
      } else if (SIGNAL_TERMINATE == signal) {
        terminateSlave(localComm);
        break;
      }
    } else { 
      int signal;
      MPI_Bcast(&signal, 1, MPI_INT, getLocalMasterRank(localComm), localComm);
      if (SIGNAL_SPLIT == signal) {
        splitSlave(localComm);
      } else if (SIGNAL_JOB == signal) {
        treatJobSlave(localComm, commands, arg.outputDir, globalTimer);         
      } else if (SIGNAL_TERMINATE == signal) {
        terminateSlave(localComm);
        break;
      }
    }
  }
  return 0;
}


SplitRanksAllocator::SplitRanksAllocator(int availableRanks,
    const string &outputDir):
  _ranksInUse(0),
  _outputDir(outputDir)
{
  Common::makedir(Common::joinPaths(outputDir, "per_job_logs"));
  MPI_Comm fakeComm;
  MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &fakeComm);
  _slots.push(Slot(0, availableRanks - 1));
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
    int signal = SIGNAL_TERMINATE;
    MPI_Send(&signal, 1, MPI_INT, _slots.front().startingRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD);
    _slots.pop();
  }
}

void split(Slot &parent, queue<Slot> &slots, int newSize)
{
  // send signal
  int signal = SIGNAL_SPLIT;
  MPI_Send(&signal, 1, MPI_INT, parent.startingRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD);
  MPI_Send(&newSize, 1, MPI_INT, parent.startingRank, TAG_SPLIT, MPI_COMM_WORLD);
  Slot newSlot;
  int slotsNumber = (parent.ranksNumber - 1) / newSize + 1; 
  for (int i = 0; i < slotsNumber; ++i) {
    int startingRank = parent.startingRank + i * newSize;
    int ranks = ((i * newSize) > parent.ranksNumber) ? max(1, newSize - 1) : newSize;
    Slot son = Slot(startingRank, ranks);
    if (i == 0) 
      newSlot = son;
    else 
      slots.push(son);
  }
  parent = newSlot;
}


InstancePtr SplitRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  Slot slot = _slots.front();
  _slots.pop();
  if (slot.ranksNumber > requestedRanks) {
    split(slot, _slots, requestedRanks); 
  }
  _ranksInUse += slot.ranksNumber;
  InstancePtr instance(new SplitInstance(_outputDir,
    slot.startingRank,
    slot.ranksNumber,
    command));
  _rankToInstances[instance->getStartingRank()] = instance;
  return instance;
}
  
void SplitRanksAllocator::freeRanks(InstancePtr instance)
{
  _ranksInUse -= instance->getRanksNumber();
  auto splitInstance = static_pointer_cast<SplitInstance>(instance);
  _slots.push(Slot(instance->getStartingRank(), 
        instance->getRanksNumber()));
}

vector<InstancePtr> SplitRanksAllocator::checkFinishedInstances()
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
    auto instance = static_pointer_cast<SplitInstance>(foundInstance->second);
    instance->setStartingElapsedMS(endJobMsg[1]);
    instance->setElapsedMs(endJobMsg[2]);
    if (endJobMsg[0]) {
      cerr << "Warning, command " << instance->getId() 
           << " failed with return code " << endJobMsg[0] << endl;
    }
    finished.push_back(instance);
  }
  if (finished.size()) {
    cout << "Check finished elapsed time " << t.getElapsedMs()  << " for " << finished.size() << " elements" << endl;
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
  if (_ranksNumber == 0) {
    throw MultiRaxmlException("Error in SplitInstance::execute: invalid number of ranks ", to_string(_ranksNumber));
  }
  int signal = SIGNAL_JOB;
  MPI_Send(&signal, 1, MPI_INT, self->getStartingRank(), TAG_MASTER_SIGNAL, MPI_COMM_WORLD);
  MPI_Send((char *)self->getId().c_str(), self->getId().size() + 1, MPI_CHAR, self->getStartingRank(), TAG_START_JOB, MPI_COMM_WORLD);
  return true;
}
  
void SplitInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  drawer.writeSquare(getStartingRank(),
    _startingElapsedMS,
    getRanksNumber(),
    getElapsedMs());
}

} // namespace MultiRaxml

