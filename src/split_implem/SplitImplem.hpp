#ifndef _MULTIRAXML_SPLIT_IMPLEM_
#define _MULTIRAXML_SPLIT_IMPLEM_

#include "../Command.hpp"
#include <queue>

namespace MultiRaxml {

int main_split_master(int argc, char **argv);
int main_split_slave(int argc, char **argv);
typedef int (*mainFct)(int,char**,void*);  
void terminate_slaves();


struct Slot {
  Slot() {}
  Slot(int _startingRank, int _ranksNumber) : 
    startingRank(_startingRank),
    ranksNumber(_ranksNumber)
  {}

  int startingRank; // relative to MPI_COMM_WORLD
  int ranksNumber;
};


class Slave {
public:
  Slave(): _handle(0) {}
  ~Slave();
  int main_split_slave(int argc, char **argv);
private:
  int loadLibrary(const string &libraryPath);
  int doWork(const CommandPtr command, MPI_Comm workersComm, const string &outputDir); 
  void splitSlave();
  void treatJobSlave();
  void terminateSlave();
  MPI_Comm _localComm; 
  int _localRank;
  int _globalRank;
  int _localMasterRank;
  int _globalMasterRank;
  CommandsContainer _commands;
  string _outputDir;
  Timer _globalTimer;
  void *_handle;
  mainFct _raxmlMain;
  string _libraryPath;
};

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
  virtual void terminate();
private:
  queue<Slot> _slots;
  int _ranksInUse;
  string _outputDir;
  map<string, InstancePtr> _runningInstances;
  map<int, InstancePtr> _rankToInstances;

  MPI_Comm _availableComms;
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
  void setStartingElapsedMS(int starting) {_startingElapsedMS = starting;}
private:
  int _startingElapsedMS;
  string _outputDir;
};

} // namespace MultiRaxml


#endif
