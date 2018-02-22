#include "MpirunImplem.hpp"
#include <iostream>
#include <mpi.h>

using namespace std;

void remove_domain(string &str)
{
  str = str.substr(0, str.find(".", 0));
}

void main_mpirun_hostfile(int argc, char** argv)
{
  MPI_Init(NULL, NULL);
  
  int name_len;
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(processor_name, &name_len);
  
  char *recieve_data = new char[MPI_MAX_PROCESSOR_NAME * world_size];

  MPI_Gather(processor_name,
      MPI_MAX_PROCESSOR_NAME,
      MPI_CHAR,
      recieve_data,
      MPI_MAX_PROCESSOR_NAME,
      MPI_CHAR,
      0,
      MPI_COMM_WORLD);
  if (!rank) {
    map<string, int> nodes;;
    for (unsigned int i = 0; i < world_size; ++i) {
      string node(recieve_data + i * MPI_MAX_PROCESSOR_NAME);
      remove_domain(node); // todobenoit this might be wrong
      int size = 0;
      if (nodes.find(node) != nodes.end()) {
        size = nodes.at(node);
      }
      nodes[node] = ++size;
    }
    for (auto pair: nodes) {
      cout << pair.first << " slots = " << pair.second << endl;
    }
  }
  delete[] recieve_data;
  MPI_Finalize();
}





MpirunRanksAllocator::MpirunRanksAllocator(int availableRanks,
    const string &outputDir):
  _outputDir(outputDir),
  _ranks(availableRanks)
{
  computePinning();
  for (unsigned int i = 0; i < availableRanks; ++i) {
    _availableRanks.insert(i); 
  }
}


bool MpirunRanksAllocator::ranksAvailable()
{
  return !_availableRanks.empty();
}
 
bool MpirunRanksAllocator::allRanksAvailable()
{
  return _availableRanks.size() == _ranks;
}
  
InstancePtr MpirunRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  int allocatedRanks = min(requestedRanks, (int)_availableRanks.size());
  Pinnings pinnings;
  auto it = _availableRanks.begin();
  for (unsigned int i = 0; i < allocatedRanks; ++i) {
    pinnings.push_back(_ranksToPinning[*it]);
    _availableRanks.erase(it++);
  }

  MpirunInstance *instance = new MpirunInstance(command,
      pinnings[0].rank,
      allocatedRanks,
      _outputDir,
      pinnings,
      *this);
  return InstancePtr(instance);
}
  
void MpirunRanksAllocator::freeRanks(InstancePtr inst)
{
  auto instance = static_pointer_cast<MpirunInstance>(inst);
  const auto &pinnings = instance->getPinnings();
  for (auto pinning: pinnings) {
    _availableRanks.insert(pinning.rank); 
  }
}


vector<InstancePtr> MpirunRanksAllocator::checkFinishedInstances()
{
  vector<InstancePtr> finished;
  for (auto running: _runningPidsToInstance) {
    int pid = running.first;
    if (!Common::isPidAlive(pid)) {
      freeRanks(running.second);
      finished.push_back(running.second);    
    }
  }
  for (auto finishedInstance: finished) {
    _runningPidsToInstance.erase(static_pointer_cast<MpirunInstance>(finishedInstance)->getPid());
  }

  return finished;
}

void MpirunRanksAllocator::computePinning() 
{
  string hostfilePath = Common::joinPaths(_outputDir, "hostfile");
  string commandPrinters = string("mpirun -np ")
    + to_string(_ranks) + string(" ") + Common::getSelfpath()
    + string (" --mpirun-hostfile ") + string("> ") + hostfilePath;
  cout << "command printer: " << commandPrinters << endl;
  system(commandPrinters.c_str());
  ifstream hostfileReader(hostfilePath);
  vector<pair<string, int> > nodes;
  cout << "hostfile saved in " << hostfilePath << endl;
  int currRank  = 0;
  while (!hostfileReader.eof()) {
    string host, temp;
    int slots;
    hostfileReader >> host;
    if (hostfileReader.eof())
      break;
    hostfileReader >> temp;
    hostfileReader >> temp;
    hostfileReader >> slots;
    for (int i = 0; i < slots; ++i) {
      _ranksToPinning[currRank] = Pinning(currRank, host);
      currRank++;
    }
  }
}
  
void fakeDelete(void *) {} // todobenoit this is a horrible hack
  
void MpirunRanksAllocator::addPid(int pid, Instance *instance)
{
  _runningPidsToInstance[pid] = shared_ptr<Instance>(instance, fakeDelete);
}
  
MpirunInstance::MpirunInstance(CommandPtr command,
  int startingRank, // todobenoit 
  int ranksNumber, 
  const string &outputDir,
  const Pinnings &pinnings,
  MpirunRanksAllocator &allocator):
  _outputDir(outputDir),
  _pinnings(pinnings),
  _allocator(allocator),
  Instance(command, startingRank, ranksNumber)
{
}

int forkAndGetPid(const string & command)
{
  int pid = fork();
  if (!pid) {
    system(command.c_str());
    exit(0);
  } else {
    return pid;
  }
}


void MpirunInstance::execute()
{

  string command;
  command += "mpirun -np ";
  command += getRanksNumber();
  const vector<string> &args = _command->getArgs();
  for (auto arg: args) {
    command += string(" ") + arg;
  }
  command += " > " + Common::joinPaths(_outputDir, getId() + ".out") + " 2>&1 "; 
  cout << "Executing command " << getId() 
       << " on nodes " << endl;
  for (auto pin: _pinnings) {
    cout << "  " <<  pin.node << endl;
  }
  _pid = forkAndGetPid(command);
  cout << "Command " << getId() << " started with pid " << _pid << endl;
}
  
void MpirunInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
}

