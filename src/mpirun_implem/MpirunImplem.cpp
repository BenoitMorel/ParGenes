#include "MpirunImplem.hpp"
#include <iostream>
#include <mpi.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <iterator>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

namespace MultiRaxml {

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
  for (unsigned int i = 0; i < _ranks; ++i) {
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
      pinnings[0].rank + 1,
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
    // todobenoit: only wait for the pids we want to wait for
    int status = 0;
    int res = waitpid(pid, &status, WNOHANG);
    if (res < -1) 
      cout << "error: wait pid " << res << endl;
    if (res != pid)
      continue;
    if (!WIFEXITED(status)) {
      cout << "Exit status error: " << status << endl;
    }
    freeRanks(running.second);
    finished.push_back(running.second);    
    _runningPidsToInstance.erase(static_pointer_cast<MpirunInstance>(running.second)->getPid());
  } 
  for (auto running: _runningPidsToInstance) {
    int pid = running.first;
    if (!Common::isPidAlive(pid)) {
      cout << "YOYO killed a lost pid" << endl;
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
  string masterHost = Common::getHost();
  remove_domain(masterHost);
  while (!hostfileReader.eof()) {
    string host, temp;
    int slots;
    hostfileReader >> host;
    if (hostfileReader.eof())
      break;
    hostfileReader >> temp;
    hostfileReader >> temp;
    hostfileReader >> slots;
    if (masterHost == host) {
      cout << "remove a slot for the host " << host << endl;
      slots--;
    }
    for (int i = 0; i < slots; ++i) {
      _ranksToPinning[currRank] = Pinning(currRank, host);
      currRank++;
    }
  }
  _ranks = currRank;
}
  
void MpirunRanksAllocator::addPid(int pid, InstancePtr instance)
{
  _runningPidsToInstance[pid] = instance;
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

void myexecv(const string &command) 
{
  istringstream iss(command);
  vector<string> tokens;
  copy(istream_iterator<string>(iss),
      istream_iterator<string>(),
      back_inserter(tokens));
  const char* exec = tokens[0].c_str();
  const char** args = new const char*[tokens.size() + 1];
  for (unsigned int i = 0; i < tokens.size(); ++i) {
    args[i] = tokens[i].c_str();
  }
  args[tokens.size()] = 0;
  int res = execv(exec, (char*const*)args);
  cerr << "error: my exec failed !!" << endl;
}

// todobenoit call execv
int forkAndGetPid(const string & command, const string &outputLogFile)
{
  int waitingTime = 1;
  for (int i = 0; i < 100; ++i) {
    while (true) {
      int pid = fork();
      if (pid == 0) {
        int fd = open(outputLogFile.c_str(), 
            O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        dup2(fd, 1);   
        dup2(fd, 2);   
        close(fd); 
        myexecv(command);
        //system(command.c_str());
        exit(0); // this line should not be called
      } else if (pid == -1) {
        cout << "Fork failed" << endl;
        return -1;
      } else {
        return pid;
      }
    }
  }
}

bool MpirunInstance::execute(InstancePtr self)
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
  command += "/hits/sw/shared/apps/OpenMPI/1.10.3-GCC-5.4.0-2.26/bin/mpirun ";
  command += " -hostfile " + Common::getHostfilePath(_outputDir);
  for (auto host: sortedHosts) { 
    cout << host.second << " -np " << host.first << ": ";
    command += " -np ";
    command += to_string(host.first);
    command += " -host ";
    command += host.second;
    command += argStr;
    command += ":";
  }
  cout << endl;
  command[command.size() - 1] = ' '; //remove the last :
  //command += " > " + Common::joinPaths(_outputDir, "per_job_logs", getId() + ".out ") + " 2>&1 "; 
  _pid = forkAndGetPid(command, Common::joinPaths(_outputDir, "per_job_logs", getId() + ".out"));
  if (_pid == -1) {
    return false;
  }
  _allocator.addPid(_pid, self);
  return true;
}
  
void MpirunInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  string hex = SVGDrawer::getRandomHex();
  for (auto &pinning: _pinnings) {
    drawer.writeSquare(pinning.rank,
      Common::getElapsedMs(initialTime, getStartTime()),
      1,
      getElapsedMs(), hex.c_str());
  }
}

void MpirunInstance::onFinished()
{
  Instance::onFinished();
  setElapsedMs(Common::getElapsedMs(_beginTime, _endTime)); 
}

} // namespace MultiRaxml

