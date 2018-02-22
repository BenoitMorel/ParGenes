#ifndef _MULTIRAXML_SPAWN_IMPLEM_
#define _MULTIRAXML_SPAWN_IMPLEM_

#include "../Command.hpp"

void main_spawned_wrapper(int argc, char** argv); 

/*
 *  This allocator assumes that each request do
 *  not ask more ranks than the previous one and
 *  that the requests are powers of 2.
 */
class SpawnedRanksAllocator: public RanksAllocator {
public:
  // available threads must be a power of 2 - 1
  SpawnedRanksAllocator(int availableRanks, 
      const string &outoutDir);
  virtual ~SpawnedRanksAllocator() {}
  virtual bool ranksAvailable();
  virtual bool allRanksAvailable();
  virtual InstancePtr allocateRanks(int requestedRanks, 
      CommandPtr command);
  virtual void freeRanks(InstancePtr instance);
  virtual vector<InstancePtr> checkFinishedInstances();
private:
  stack<std::pair<int, int> > _slots;
  int _ranksInUse;
  string _outputDir;
  map<string, InstancePtr> _startedInstances;
};

class SpawnInstance: public Instance {
public:
  SpawnInstance(const string &outputDir, 
      int startingRank, 
      int ranksNumber,
      CommandPtr command);

  virtual ~SpawnInstance() {}
  virtual void execute();
  virtual void writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime); 

private:
  string _outputDir;
};


#endif
