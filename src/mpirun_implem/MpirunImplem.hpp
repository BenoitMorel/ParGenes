#ifndef _MULTIRAXML_MPIRUN_IMPLEM_
#define _MULTIRAXML_MPIRUN_IMPLEM_

#include "../Command.hpp"
#include <map>
#include <vector>

void main_mpirun_hostfile(int argc, char** argv);

struct Pinning {
  Pinning() {}
  Pinning(int myrank, const string &n): rank(myrank), node(n) {}
  int rank;
  string node;  
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
private:
  void computePinning();
  string _outputDir;
  unsigned int _ranks;
  map<int, Pinning> _ranksToPinning;
};

class MpirunInstance: public Instance {
public:
  MpirunInstance(CommandPtr command,
      int startingRank, // todobenoit 
      int ranksNumber, 
      const string &outputDir,
      const Pinnings &pinnings);

  virtual ~MpirunInstance() {}
  
  virtual void execute();
  
  virtual void writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime); 

private:
  string _outputDir;
  Pinnings _pinnings;
};


#endif

