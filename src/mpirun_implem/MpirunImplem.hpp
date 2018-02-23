#ifndef _MULTIRAXML_MPIRUN_IMPLEM_
#define _MULTIRAXML_MPIRUN_IMPLEM_

#include "../Command.hpp"
#include <map>
#include <set>
#include <vector>

void main_mpirun_hostfile(int argc, char** argv);

struct Pinning {
  Pinning() {}
  Pinning(int myrank, const string &n): rank(myrank), host(n) {}
  int rank;
  string host;  
};

using Pinnings = vector<Pinning>;

class MpirunRanksAllocator: public RanksAllocator {
public:
  // available threads must be a power of 2 - 1
  MpirunRanksAllocator(int availableRanks, 
      const string &outoutDir);
  virtual ~MpirunRanksAllocator() {}
  virtual bool ranksAvailable();
  virtual bool allRanksAvailable();
  virtual InstancePtr allocateRanks(int requestedRanks, 
      CommandPtr command);
  virtual void freeRanks(InstancePtr instance);
  virtual vector<InstancePtr> checkFinishedInstances();
  void addPid(int pid, Instance *instance);
private:
  void computePinning();
  string _outputDir;
  unsigned int _ranks;

  map<int, Pinning> _ranksToPinning;
  set<int> _availableRanks; 
  map<int, InstancePtr> _runningPidsToInstance;
};

class MpirunInstance: public Instance {
public:
  MpirunInstance(CommandPtr command,
      int startingRank, // todobenoit 
      int ranksNumber, 
      const string &outputDir,
      const Pinnings &pinnings,
      MpirunRanksAllocator &allocator);

  virtual ~MpirunInstance() {}
  
  virtual void execute();
  
  virtual void writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime); 

  const Pinnings &getPinnings() const {return _pinnings;}
  
  int getPid() const {return _pid;}

  virtual void onFinished();
private:
  string _outputDir;
  Pinnings _pinnings;
  int _pid;
  MpirunRanksAllocator &_allocator;
};


#endif

