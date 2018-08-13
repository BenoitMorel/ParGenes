#ifndef _MULTIRAXML_COMMAND_HPP_
#define _MULTIRAXML_COMMAND_HPP_

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <stack>
#include "Common.hpp"
#include "Checkpoint.hpp"

using namespace std;

namespace MPIScheduler {

class SVGDrawer;

class Command {
public:
  Command(const string &id, 
      unsigned int ranks,
      long estimatedCost,
      const vector<string> &arguments);
  virtual ~Command() {}
 
  const string &getId() const {return _id;}
  long getEstimatedCost() const {return _estimatedCost;}
  void setRanksNumber(int ranks) {_ranksNumber = ranks;}
  int getRanksNumber() const {return _ranksNumber;}
  string toString() const;
  const vector<string> &getArgs() const {return _args;}
public:
  // initial information
  const string _id;
  const vector<string> _args;
  int _ranksNumber;
  long _estimatedCost;
};
using CommandPtr = shared_ptr<Command>;

class CommandsContainer {
public:
  CommandsContainer() {}
  explicit CommandsContainer(const string &commandsFilename);

  CommandPtr getCommand(string id) const;
  vector<CommandPtr> &getCommands() {return _commands;}
  const vector<CommandPtr> &getCommands() const {return _commands;}
private:
  void addCommand(CommandPtr command);

  vector<CommandPtr> _commands;
  map<string, CommandPtr> _dicoCommands;
};

class Instance {
public:
  Instance(CommandPtr command, int startingRank, int ranksNumber, const string &outputDir); 
  virtual bool execute(shared_ptr<Instance> self) = 0;
  virtual void writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime); 
  const string &getId() const {return _command->getId();}
  virtual void onFinished();
  int getStartingRank() const {return _startingRank;} 
  int getRanksNumber() const {return _ranksNumber;} 
  void setElapsedMs(int elapsed) {_elapsed = elapsed;}
  virtual int getElapsedMs() const {return _elapsed;}
  void setStartingElapsedMS(int starting) {_startingElapsedMS = starting;}
protected:
  const string &getOutputDir() {return _outputDir;}
  
  int _startingElapsedMS;
  string _outputDir;
  CommandPtr _command;
  int _startingRank;
  int _ranksNumber;
  Time _endTime;
  int _elapsed;
};
using InstancePtr = shared_ptr<Instance>;
using InstancesHistoric = vector<InstancePtr>;

class RanksAllocator {
public:
  virtual ~RanksAllocator() {};
  virtual bool ranksAvailable() = 0;
  virtual bool allRanksAvailable() = 0;
  virtual InstancePtr allocateRanks(int requestedRanks, 
      CommandPtr command) = 0;
  virtual void freeRanks(InstancePtr instance) = 0;
  virtual vector<InstancePtr> checkFinishedInstances() = 0;
  virtual void terminate() {}
  virtual void preprocessCommand(CommandPtr cmd) {}
};

class CommandsRunner {
public:
  CommandsRunner(const CommandsContainer &commandsContainer,
      shared_ptr<RanksAllocator> allocator,
      const string &outputDir);
  void run();
  void runOpenMP();
  const InstancesHistoric &getHistoric() const {return _historic;} 
private:
  
  static bool compareCommands(CommandPtr c1, CommandPtr c2);
  CommandPtr getPendingCommand() {return *_commandIterator;}
  bool isCommandsEmpty() {return _commandIterator == _commandsVector.end();}
  
  bool executePendingCommand();
  void onFinishedInstance(InstancePtr instance);
  
  const CommandsContainer &_commandsContainer;
  
  const string _outputDir;

  shared_ptr<RanksAllocator> _allocator;
  vector<CommandPtr> _commandsVector;
  vector<CommandPtr>::iterator _commandIterator;
  Checkpoint _checkpoint;
  InstancesHistoric _historic;
  int _finishedInstancesNumber;
  bool _verbose;
};

class RunStatistics {
public:
  RunStatistics(const InstancesHistoric &historic, 
      Time begin, 
      Time end,
      int availableRanks);
  void printGeneralStatistics();
  void exportSVG(const string &svgfile);
private:
  const InstancesHistoric &_historic;
  Time _begin;
  Time _end;
  int _availableRanks;
  double _lbRatio;
};

} // namespace MPIScheduler

#endif
