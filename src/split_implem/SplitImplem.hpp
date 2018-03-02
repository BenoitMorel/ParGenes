#ifndef _MULTIRAXML_SPLIT_IMPLEM_
#define _MULTIRAXML_SPLIT_IMPLEM_

#include "../Command.hpp"

int main_split_master(int argc, char **argv);
int main_split_slave(int argc, char **argv);

/*
 *  This allocator assumes that each request do
 *  not ask more ranks than the previous one and
 *  that the requests are powers of 2.
 */
class SplitRanksAllocator: public RanksAllocator {
public:
  // available threads must be a power of 2
  SplitRanksAllocator(int availableRanks, 
      const string &outoutDir);
  virtual ~SplitRanksAllocator() {}
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

class SplitInstance: public Instance {
public:
  SplitInstance(const string &outputDir, 
      int startingRank, 
      int ranksNumber,
      CommandPtr command);

  virtual ~SplitInstance() {}
  virtual bool execute(InstancePtr self);
  virtual void writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime); 

private:
  string _outputDir;
};


#endif
