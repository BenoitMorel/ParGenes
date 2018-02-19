#ifndef _MULTIRAXML_COMMAND_HPP_
#define _MULTIRAXML_COMMAND_HPP_

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <stack>
#include "Common.hpp"

using namespace std;

class Command {
public:
  Command(const string &id, 
      bool isMpiCommand,
      unsigned int ranks,
      unsigned int estimatedCost,
      const vector<string> &arguments);
  virtual ~Command() {}
 
  const string &getId() const {return _id;}
  int getEstimatedCost() const {return _estimatedCost;}
  int getRanksNumber() const {return _ranksNumber;}
  bool isMpiCommand() const {return _isMpiCommand;}
  string toString() const;
  const vector<string> &getArgs() const {return _args;}
public:
  // initial information
  const string _id;
  const vector<string> _args;
  bool _isMpiCommand;
  int _ranksNumber;
  int _estimatedCost;
};
using CommandPtr = shared_ptr<Command>;

class CommandsContainer {
public:
  CommandsContainer(const string &commandsFilename);

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
  Instance(const string &outputDir, 
      int startingRank, 
      int ranksNumber,
      CommandPtr command);

  void execute();
  const string &getId() const {return _command->getId();}
  bool didFinish() const {return _finished;}
  void onFinished();
  void setRanksNumber(int ranksNumber) {_ranksNumber = ranksNumber;}
  int getRanksNumber() const {return _ranksNumber;}
  Time getStartTime() const {return _beginTime;} 
  int getElapsedMs() const {return Common::getElapsedMs(_beginTime, _endTime);}
  int getStartRank() const {return _startingRank;} 
private:
  string _outputDir;
  int _startingRank;
  int _ranksNumber;
  CommandPtr _command;
  Time _beginTime;
  Time _endTime;
  bool _finished;
};
using InstancePtr = shared_ptr<Instance>;
using InstancesHistoric = vector<InstancePtr>;

/*
 *  This allocator assumes that each request do
 *  not ask more ranks than the previous one and
 *  that the requests are powers of 2.
 */
class RanksAllocator {
public:
  // available threads must be a power of 2 - 1
  RanksAllocator(int availableRanks);
  bool ranksAvailable();
  bool allRanksAvailable();
  void allocateRanks(int requestedRanks, 
      int &startingThread,
      int &threadsNumber);
  void freeRanks(int startingRank, int ranksNumber);
private:
  stack<std::pair<int, int> > _slots;
  int _ranksInUse;
};

class CommandsRunner {
public:
  CommandsRunner(const CommandsContainer &commandsContainer,
      unsigned int availableRanks,
      const string &outputDir);
  void run();
  const InstancesHistoric &getHistoric() const {return _historic;} 
private:
  
  static bool compareCommands(CommandPtr c1, CommandPtr c2);
  CommandPtr getPendingCommand() {return *_commandIterator;}
  bool isCommandsEmpty() {return _commandIterator == _commandsVector.end();}
  const string &getOutputDir() const {return _outputDir;}

  void executePendingCommand();
  void checkCommandsFinished();
  void onInstanceFinished(InstancePtr instance);
  
  const CommandsContainer &_commandsContainer;
  
  const unsigned int _availableRanks;
  const string _outputDir;

  RanksAllocator _allocator;
  vector<CommandPtr> _commandsVector;
  vector<CommandPtr>::iterator _commandIterator;
  map<string, InstancePtr> _startedInstances;
  InstancesHistoric _historic;
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
};

#endif
