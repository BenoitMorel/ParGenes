#ifndef _MULTIRAXML_MPIRUN_IMPLEM_
#define _MULTIRAXML_MPIRUN_IMPLEM_

#include "../Command.hpp"

/*
 *  This allocator assumes that each request do
 *  not ask more ranks than the previous one and
 *  that the requests are powers of 2.
 */
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
  string _outputDir;
  map<string, InstancePtr> _startedInstances;
};

class MpirunInstance: public Instance {
public:
  virtual ~MpirunInstance() {}
  
  virtual void execute();
  
  virtual void writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime); 

private:
  string _outputDir;
};


#endif

