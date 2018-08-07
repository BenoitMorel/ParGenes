#ifndef ONECORE_IMPLEM_HPP
#define ONECORE_IMPLEM_HPP

#include "../Command.hpp"
#include <queue>


namespace MPIScheduler {

class OneCoreSlave {
public:
  OneCoreSlave():
    _masterRank(-1)
  {}
  ~OneCoreSlave();
  int main_core_slave(int argc, char **argv);

  int doWork(const CommandPtr command, 
    const string &outputDir) ;
private:
  void treatJobSlave();
  string _execPath;
  string _outputDir;
  int _masterRank;
  Timer _globalTimer;
  CommandsContainer _commands;
};

/*
 *  This allocator only returns single ranks
 */
class OneCoreRanksAllocator: public RanksAllocator {
public:
  // available threads must be a power of 2
  OneCoreRanksAllocator(int availableRanks, 
      const string &outoutDir);
  virtual ~OneCoreRanksAllocator() {}
  virtual bool ranksAvailable();
  virtual bool allRanksAvailable();
  virtual InstancePtr allocateRanks(int requestedRanks, 
      CommandPtr command);
  virtual void freeRanks(InstancePtr instance);
  virtual vector<InstancePtr> checkFinishedInstances();
  virtual void terminate();
private:
  int _cores;
  queue<int> _availableCores;
  string _outputDir;
  map<int, InstancePtr> _rankToInstances;

};

class OneCoreInstance: public Instance {
public:
  OneCoreInstance(const string &outputDir, 
      int rank, 
      CommandPtr command);

  virtual ~OneCoreInstance() {}
  virtual bool execute(InstancePtr self);
};

} // namespace MPIScheduler


#endif

