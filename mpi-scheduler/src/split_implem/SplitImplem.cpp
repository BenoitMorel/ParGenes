#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>

namespace MPIScheduler {

const int SIGNAL_SPLIT = 1;
const int SIGNAL_JOB = 2;
const int SIGNAL_TERMINATE = 3;

const int TAG_END_JOB = 1;
const int TAG_START_JOB = 2;
const int TAG_SPLIT = 3;
const int TAG_MASTER_SIGNAL = 4;

const int MSG_SIZE_END_JOB = 3;

Slave::~Slave()
{
  if (_handle)
    dlclose(_handle);
}

int Slave::loadLibrary(const string &libraryPath)
{
  _libraryPath = libraryPath;
  if  (_handle) {
    dlclose(_handle);
  }
  _handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
  if (!_handle) {
    cerr << "Cannot open shared library " << libraryPath << endl;
    return 1;
  }
  _raxmlMain = (mainFct) dlsym(_handle, "dll_main");
  const char *dlsym_error = dlerror();
  if (dlsym_error) {
    cerr << "Cannot load symbole dll_main " << dlsym_error << endl;
    dlclose(_handle);
    _handle = 0;
    return 1;
  }
  return 0;
}


int Slave::doWork(const CommandPtr command, 
    MPI_Comm workersComm,
    const string &outputDir) 
{
  loadLibrary(_libraryPath);
  std::ofstream out(Common::joinPaths(outputDir, "per_job_logs", command->getId() + "_out.txt"));
  std::streambuf *coutbuf = std::cout.rdbuf(); 
  std::cout.rdbuf(out.rdbuf()); 
  const vector<string> &args  = command->getArgs();
  int argc = args.size(); 
  char **argv = new char*[argc];
  for (int i = 0; i < argc; ++i) {
    argv[i] = (char*)args[i].c_str();
    cout << argv[i] << " ";
  }
  cout << endl;
  int res = _raxmlMain(argc, argv, (void*)&workersComm);
  delete[] argv;
  MPI_Barrier(workersComm);
  std::cout.rdbuf(coutbuf); 
  return res;
}


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


void Slave::splitSlave() 
{
  MPI_Status status;
  int splitSize;
  if (_localMasterRank == _localRank) {
    MPI_Recv(&splitSize, 1, MPI_INT, _globalMasterRank, TAG_SPLIT, MPI_COMM_WORLD, &status);
    int signal = SIGNAL_SPLIT;
    MPI_Bcast(&signal, 1, MPI_INT, _localMasterRank, _localComm);
  }
  MPI_Bcast(&splitSize, 1, MPI_INT, _localMasterRank, _localComm);
  bool inFirstSplit = (_localRank < splitSize);
  int newRank = inFirstSplit ? _localRank : (_localRank - splitSize);
  MPI_Comm newComm;
  MPI_Comm_split(_localComm, !inFirstSplit , newRank, &newComm);
  _localComm = newComm;
  _localRank = newRank;
}

void Slave::treatJobSlave()
{
  bool alone = getSize(_localComm) == 1;
  MPI_Status status;
  const int maxCommandSize = 200;
  char command[maxCommandSize];
  if (_localMasterRank == _localRank) {
    MPI_Recv(command, maxCommandSize, MPI_CHAR, _globalMasterRank, TAG_START_JOB, MPI_COMM_WORLD, &status);
    if (!alone) {
      int signal = SIGNAL_JOB;
      MPI_Bcast(&signal, 1, MPI_INT, _localMasterRank, _localComm);
    }
  }

  if (!alone) {
    MPI_Bcast(command, maxCommandSize, MPI_CHAR, _localMasterRank, _localComm);
  }
  Timer timer;
  int startingTime = _globalTimer.getElapsedMs();
  int jobResult = doWork(_commands.getCommand(string(command)), _localComm, _outputDir);
  int elapsedMS = timer.getElapsedMs();
  if (_localMasterRank == _localRank) {
    int endJobMsg[MSG_SIZE_END_JOB];
    endJobMsg[0] = jobResult;
    endJobMsg[1] = startingTime;
    endJobMsg[2] = elapsedMS;
    MPI_Send(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, _globalMasterRank, TAG_END_JOB, MPI_COMM_WORLD);
  }
}

void Slave::terminateSlave()
{
  if (_localMasterRank == _localRank) {
    int signal = SIGNAL_TERMINATE;
    MPI_Bcast(&signal, 1, MPI_INT, _localMasterRank, _localComm);
  }
  MPI_Finalize();
}

int Slave::main_split_slave(int argc, char **argv)
{
  SchedulerArgumentsParser arg(argc, argv);
  if (loadLibrary(arg.library)) {
    return 1;
  }
  _outputDir = arg.outputDir;
  _commands = CommandsContainer(arg.commandsFilename);
  _globalRank = getRank(MPI_COMM_WORLD);
  _localMasterRank = 0;
  _globalMasterRank = getSize(MPI_COMM_WORLD) - 1;
  _localRank = _globalRank;
  MPI_Comm_split(MPI_COMM_WORLD, 1, _localRank, &_localComm);
  while (true) {
    _localRank = getRank(_localComm);
    if (!_localRank) {
      int signal;
      MPI_Status status;
      MPI_Recv(&signal, 1, MPI_INT, _globalMasterRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD, &status);
      if (SIGNAL_SPLIT == signal) {
        splitSlave();
      } else if (SIGNAL_JOB == signal) {
        treatJobSlave();         
      } else if (SIGNAL_TERMINATE == signal) {
        terminateSlave();
        break;
      }
    } else { 
      int signal;
      MPI_Bcast(&signal, 1, MPI_INT, _localMasterRank, _localComm);
      if (SIGNAL_SPLIT == signal) {
        splitSlave();
      } else if (SIGNAL_JOB == signal) {
        treatJobSlave();         
      } else if (SIGNAL_TERMINATE == signal) {
        terminateSlave();
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

void split(const Slot &parent,
    Slot &son1,
    Slot &son2,
    int son1size)
{
  // send signal
  int signal = SIGNAL_SPLIT;
  MPI_Send(&signal, 1, MPI_INT, parent.startingRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD);
  MPI_Send(&son1size, 1, MPI_INT, parent.startingRank, TAG_SPLIT, MPI_COMM_WORLD);
  son1 = Slot(parent.startingRank, son1size);
  son2 = Slot(parent.startingRank + son1size, parent.ranksNumber - son1size);
}


InstancePtr SplitRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  Slot slot = _slots.front();
  _slots.pop();
  // border case around the first rank
  while (slot.ranksNumber > requestedRanks) {
    Slot slot1, slot2;
    split(slot, slot1, slot2, slot.ranksNumber / 2); 
    slot = slot1;
    _slots.push(slot2);
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
    //_rankToInstances.erase(source);

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
    throw MPISchedulerException("Error in SplitInstance::execute: invalid number of ranks ", to_string(_ranksNumber));
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

} // namespace MPIScheduler

