#include "MpirunImplem.hpp"
#include <iostream>
#include <mpi.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>

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
  int pid;
  while(true) {
    pid = waitpid(WAIT_ANY, NULL, WNOHANG);
    if (pid <= 0)
      break;
    auto instIt = _runningPidsToInstance.find(pid);
    if (instIt == _runningPidsToInstance.end())
      continue;
    freeRanks(instIt->second);
    finished.push_back(instIt->second);    
    _runningPidsToInstance.erase(static_pointer_cast<MpirunInstance>(instIt->second)->getPid());
  } 
  return finished;
}

void MpirunRanksAllocator::computePinning() 
{
  string hostfilePath = Common::getHostfilePath(_outputDir);
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
  Common::makedir(Common::joinPaths(_outputDir, "per_job_logs"));
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
  _beginTime = Common::getTime();
  map<string, int> hosts;
  for (auto pinning: _pinnings) {
    auto entry = hosts.find(pinning.host);
    int value = 0;
    if (entry != hosts.end()) 
      value = entry->second;
    hosts[pinning.host] = value + 1;
  }

  //todobenoit the sorting is maybe useless (at least with the current mpirun call syntax)
  vector<pair<int, string> >sortedHosts;
  for (auto host: hosts) {
    sortedHosts.push_back(pair<int, string>(host.second, host.first));
  }
  sort(sortedHosts.begin(), sortedHosts.end());
  reverse(sortedHosts.begin(), sortedHosts.end());

  const vector<string> &args = _command->getArgs();
  string argStr;
  for (auto arg: args) {
    argStr += string(" ") + arg;
  }

  string command;
  command += "mpirun ";
  command += " -hostfile " + Common::getHostfilePath(_outputDir);
  for (auto host: sortedHosts) { 
    command += " -np ";
    command += to_string(host.first);
    command += " -host ";
    command += host.second;
    command += argStr;
    command += ":";
  }
  command[command.size() - 1] = ' '; //remove the last :
  command += " > " + Common::joinPaths(_outputDir, "per_job_logs", getId() + ".out ") + " 2>&1 "; 
  _pid = forkAndGetPid(command);
  _allocator.addPid(_pid, this);
}
  
void MpirunInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
}

void MpirunInstance::onFinished()
{
  Instance::onFinished();
  setElapsedMs(Common::getElapsedMs(_beginTime, _endTime)); 
}
