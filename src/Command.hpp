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
  Command(const string &id, const string &command);
  virtual ~Command() {}
 
  const string &getId() const {return _id;}
  const string &getCommand() const {return _command;}
  void setRanksNumber(int ranksNumber) {_ranksNumber = ranksNumber;}
  int getRanksNumber() const {return _ranksNumber;}
  string toString() const;
  
  void execute(const string &outputDir, int startingRank, int ranksNumber);
  void onFinished();
  Time getStartTime() const {return _beginTime;} 
  int getElapsedMs() const {return Common::getElapsedMs(_beginTime, _endTime);}
  int getStartRank() const {return _startRank;} 
public:
  // initial information
  const string _id;
  const string _command;
  int _ranksNumber;
  
  // execution information
  int _startRank;
  Time _beginTime;
  Time _endTime;
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
private:
  
  CommandPtr getPendingCommand() {return *_commandIterator;}
  bool isCommandsEmpty() {return _commandIterator == _commandsVector.end();}
  const string &getOutputDir() const {return _outputDir;}
  

  void executePendingCommand();
  void checkCommandsFinished();
  void onCommandFinished(CommandPtr command);
  
  const CommandsContainer &_commandsContainer;
  
  const unsigned int _availableRanks;
  const string _outputDir;
  
  RanksAllocator _allocator;
  vector<CommandPtr> _commandsVector;
  vector<CommandPtr>::iterator _commandIterator;
};

class CommandsStatistics {
public:
  CommandsStatistics(const CommandsContainer &commands, 
      Time begin, 
      Time end,
      int availableRanks);
  void printGeneralStatistics();
  void exportSVG(const string &svgfile);
private:
  const CommandsContainer &_commands;
  Time _begin;
  Time _end;
  int _availableRanks;
};

#endif
