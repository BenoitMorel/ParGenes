#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>
#include <fstream>
#include <stdio.h>

namespace MPIScheduler {

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
  Common::check(MPI_Comm_rank(comm, &rank));
  return rank;
}
int getSize(MPI_Comm comm) {
  int size = 0;
  Common::check(MPI_Comm_size(comm, &size));
  return size;
}


int mpiSend(void* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
  //cout << "mpisend tag=" << tag << " dest=" << destination << " rank=" << getRank(MPI_COMM_WORLD) << endl << flush;
  return MPI_Send(data, count, datatype, destination, tag, communicator);
}

int mpiRecv(        void* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
{
  int res =  MPI_Recv(data, count, datatype, source, tag, communicator, status);
  //cout << "mpirecv tag=" << tag << " src=" << source << " rank=" << getRank(MPI_COMM_WORLD) << endl << flush;
  return res;
}

int mpiBcast( void *buffer, int count, MPI_Datatype datatype, int root, 
                   MPI_Comm comm)
{
  //cout << "begin bcast " << root << " " << getRank(MPI_COMM_WORLD) << endl << flush;
  int res = MPI_Bcast(buffer, count, datatype, root, comm);
  //cout << "end  bcast " << root << " " << getRank(MPI_COMM_WORLD) << endl << flush;
  return res;
}

int mpiIprobe(int source, int tag, MPI_Comm comm, int *flag,
        MPI_Status *status)
{
  return MPI_Iprobe(source, tag, comm, flag, status);
}

int mpiSplit(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
  return MPI_Comm_split(comm, color, key, newcomm);
}

SplitSlave::~SplitSlave()
{
  if (_handle)
    dlclose(_handle);
}

int SplitSlave::loadLibrary(const string &libraryPath)
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


int SplitSlave::doWork(const CommandPtr command, 
    MPI_Comm workersComm,
    const string &outputDir) 
{
  bool isMaster = !getRank(workersComm);
  string logsFile = Common::joinPaths(outputDir, "per_job_logs", command->getId() + "_out.txt");
  string errFile = Common::joinPaths(outputDir, "per_job_logs", command->getId() + "_err.txt");
  string runningFile = Common::joinPaths(outputDir, "running_jobs", command->getId());
  if (isMaster) {
    ofstream os(runningFile);
    os << logsFile << endl;
    os.close();
  }
  loadLibrary(_libraryPath);
  std::ofstream out(logsFile);
  std::streambuf *coutbuf = std::cout.rdbuf(); 
  std::cout.rdbuf(out.rdbuf()); 
  std::ofstream err(errFile);
  std::streambuf *cerrbuf = std::cerr.rdbuf(); 
  std::cerr.rdbuf(err.rdbuf()); 
  const vector<string> &args  = command->getArgs();
  int argc = args.size(); 
  char **argv = new char*[argc];
  for (int i = 0; i < argc; ++i) {
    argv[i] = (char*)args[i].c_str();
    cout << argv[i] << " ";
  }
  cout << endl;
  //Common::check(MPI_Barrier(workersComm));
  //MPI_Comm_set_errhandler(workersComm, MPI_ERRORS_RETURN);
  MPI_Comm raxmlComm;
  Common::check(MPI_Comm_dup(workersComm, &raxmlComm));
  int res = _raxmlMain(argc, argv, (void*)&raxmlComm);
  std::cout.rdbuf(coutbuf);
  std::cerr.rdbuf(cerrbuf);
  delete[] argv;
  Common::check(MPI_Barrier(raxmlComm));
  MPI_Comm_free(&raxmlComm);
  if (isMaster) {
    remove(runningFile.c_str());
  }
  return res;
}


void SplitSlave::splitSlave() 
{
  MPI_Status status;
  int splitSize;
  if (_localMasterRank == _localRank) {
    Common::check(mpiRecv(&splitSize, 1, MPI_INT, _globalMasterRank, TAG_SPLIT, MPI_COMM_WORLD, &status));
    int signal = SIGNAL_SPLIT;
    Common::check(mpiBcast(&signal, 1, MPI_INT, _localMasterRank, _localComm));
  }
  Common::check(mpiBcast(&splitSize, 1, MPI_INT, _localMasterRank, _localComm));
  bool inFirstSplit = (_localRank < splitSize);
  int newRank = inFirstSplit ? _localRank : (_localRank - splitSize);
  MPI_Comm newComm;
  Common::check(mpiSplit(_localComm, !inFirstSplit , newRank, &newComm));
  _localComm = newComm;
  _localRank = newRank;
}

void SplitSlave::treatJobSlave()
{
  bool alone = getSize(_localComm) == 1;
  MPI_Status status;
  const int maxCommandSize = 200;
  char command[maxCommandSize];
  if (_localMasterRank == _localRank) {
    Common::check(mpiRecv(command, maxCommandSize, MPI_CHAR, _globalMasterRank, TAG_START_JOB, MPI_COMM_WORLD, &status));
    if (!alone) {
      int signal = SIGNAL_JOB;
      Common::check(mpiBcast(&signal, 1, MPI_INT, _localMasterRank, _localComm));
    }
  }

  if (!alone) {
    Common::check(mpiBcast(command, maxCommandSize, MPI_CHAR, _localMasterRank, _localComm));
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
    Common::check(mpiSend(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, _globalMasterRank, TAG_END_JOB, MPI_COMM_WORLD));
  }
}

void SplitSlave::terminateSlave()
{
  if (_localMasterRank == _localRank) {
    int signal = SIGNAL_TERMINATE;
    Common::check(mpiBcast(&signal, 1, MPI_INT, _localMasterRank, _localComm));
  }
  Common::check(MPI_Finalize());
}

int SplitSlave::main_split_slave(int argc, char **argv)
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
  // split master and slave
  Common::check(mpiSplit(MPI_COMM_WORLD, 1, _localRank, &_localComm));
  while (true) {
    _localRank = getRank(_localComm);
    if (!_localRank) {
      int signal;
      MPI_Status status;
      Common::check(mpiRecv(&signal, 1, MPI_INT, _globalMasterRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD, &status));
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
      Common::check(mpiBcast(&signal, 1, MPI_INT, _localMasterRank, _localComm));
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
  _totalRanks(availableRanks),
  _ranksInUse(0),
  _outputDir(outputDir)
{
  Common::makedir(Common::joinPaths(outputDir, "per_job_logs"));
  Common::makedir(Common::joinPaths(outputDir, "running_jobs"));
  MPI_Comm fakeComm;
  // split master and slave
  Common::check(mpiSplit(MPI_COMM_WORLD, 0, 0, &fakeComm));
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
    Common::check(mpiSend(&signal, 1, MPI_INT, _slots.front().startingRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD));
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
  Common::check(mpiSend(&signal, 1, MPI_INT, parent.startingRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD));
  Common::check(mpiSend(&son1size, 1, MPI_INT, parent.startingRank, TAG_SPLIT, MPI_COMM_WORLD));
  son1 = Slot(parent.startingRank, son1size);
  son2 = Slot(parent.startingRank + son1size, parent.ranksNumber - son1size);
}


InstancePtr SplitRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  Slot slot = _slots.front();
  _slots.pop();
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
    Common::check(mpiIprobe(MPI_ANY_SOURCE, TAG_END_JOB, MPI_COMM_WORLD, &flag, &status)); 
    if (!flag) {
      break;
    }
    int source = status.MPI_SOURCE;
    int endJobMsg[MSG_SIZE_END_JOB];
    Common::check(mpiRecv(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, source,  
        TAG_END_JOB, MPI_COMM_WORLD, &status));
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
  return finished;
}

void SplitRanksAllocator::preprocessCommand(CommandPtr cmd)
{
  int ranksNumber = cmd->getRanksNumber();
  int newNumber = _totalRanks;
  while (newNumber > ranksNumber) {
    newNumber /= 2;
  }
  cmd->setRanksNumber(newNumber);
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
  Common::check(mpiSend(&signal, 1, MPI_INT, self->getStartingRank(), TAG_MASTER_SIGNAL, MPI_COMM_WORLD));
  Common::check(mpiSend((char *)self->getId().c_str(), self->getId().size() + 1, MPI_CHAR, self->getStartingRank(), TAG_START_JOB, MPI_COMM_WORLD));
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

