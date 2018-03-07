#include "SplitImplem.hpp"
#include <mpi.h>
#include <iostream>
#include <dlfcn.h>

namespace MultiRaxml {

const int MASTER_RANK = 0;
const int SIGNAL_SPLIT = 1;
const int SIGNAL_JOB = 2;
const int SIGNAL_TERMINATE = 3;

const int TAG_END_JOB = 1;
const int TAG_START_JOB = 2;
const int TAG_SPLIT = 3;
const int TAG_MASTER_SIGNAL = 4;

const int MSG_SIZE_END_JOB = 3;



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


void splitSlave(MPI_Comm &localComm) 
{
  int localRank = getRank(localComm);
  MPI_Status status;
  int splitSize;
  if (MASTER_RANK == getRank(localComm)) {
    MPI_Recv(&splitSize, 1, MPI_INT, MASTER_RANK, TAG_SPLIT, MPI_COMM_WORLD, &status);
    int signal = SIGNAL_SPLIT;
    MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, localComm);
  }
  MPI_Bcast(&splitSize, 1, MPI_INT, MASTER_RANK, localComm);
  bool inFirstSplit = (localRank < splitSize);
  int newRank = inFirstSplit ? localRank : (localRank - splitSize);
  MPI_Comm newComm;
  MPI_Comm_split(localComm, !inFirstSplit , newRank, &newComm);
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
  if (MASTER_RANK == getRank(localComm)) {
    MPI_Recv(command, maxCommandSize, MPI_CHAR, MASTER_RANK, TAG_START_JOB, MPI_COMM_WORLD, &status);
    if (!alone) {
      int signal = SIGNAL_JOB;
      MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, localComm);
    }
  }

  if (!alone) {
    MPI_Bcast(command, maxCommandSize, MPI_CHAR, MASTER_RANK, localComm);
  }
  Timer timer;
  int startingTime = globalTimer.getElapsedMs();
  int jobResult = doWork(commands.getCommand(string(command)), localComm, outputDir);
  int elapsedMS = timer.getElapsedMs();
  if (MASTER_RANK == getRank(localComm)) {
    int endJobMsg[MSG_SIZE_END_JOB];
    endJobMsg[0] = jobResult;
    endJobMsg[1] = startingTime;
    endJobMsg[2] = elapsedMS;
    MPI_Send(endJobMsg, MSG_SIZE_END_JOB, MPI_INT, MASTER_RANK, TAG_END_JOB, MPI_COMM_WORLD);
  }
}

void terminateSlave(MPI_Comm localComm)
{
  if (MASTER_RANK == getRank(localComm)) {
    int signal = SIGNAL_TERMINATE;
    MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, localComm);
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
  MPI_Comm_split(MPI_COMM_WORLD, 1, getRank(MPI_COMM_WORLD) - 1, &localComm);
  int globalRank = getRank(MPI_COMM_WORLD);
  while (true) {
    if (!getRank(localComm)) {
      int signal;
      MPI_Status status;
      MPI_Recv(&signal, 1, MPI_INT, MASTER_RANK, TAG_MASTER_SIGNAL, MPI_COMM_WORLD, &status);
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
      MPI_Bcast(&signal, 1, MPI_INT, MASTER_RANK, localComm);
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
  _slots.push(Slot(1, availableRanks - 1));
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
    MPI_Send(&signal, 1, MPI_INT, _slots.top().startingRank, TAG_MASTER_SIGNAL, MPI_COMM_WORLD);
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

