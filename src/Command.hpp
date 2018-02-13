#ifndef _MULTIRAXML_COMMAND_HPP_
#define _MULTIRAXML_COMMAND_HPP_

#include <string>
#include <memory>
#include <vector>
#include <map>
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

  void execute(const string &outputDir);
  int getElapsedMs() const {return _timer.getElapsedMs();}
  string toString() const;
public:
  string _id;
  string _command;
  int _ranksNumber;
  Timer _timer;
};

using CommandPtr = shared_ptr<Command>;

class CommandsContainer {
public:
  CommandsContainer(const string &commandsFilename);

  CommandPtr getCommand(string id) const;
  vector<CommandPtr> &getCommands() {return _commands;}
private:
  void addCommand(CommandPtr command);

  vector<CommandPtr> _commands;
  map<string, CommandPtr> _dicoCommands;
};

class CommandsRunner {
public:
  CommandsRunner(CommandsContainer &commandsContainer,
      unsigned int availableThreads,
      const string &outputDir);
  void run();
private:
  
  CommandPtr getPendingCommand() {return *_commandIterator;}
  bool isCommandsEmpty() {return _commandIterator == _commandsContainer.getCommands().end();}
  const string &getOutputDir() const {return _outputDir;}
  

  void executePendingCommand();
  void checkCommandsFinished();
  void onCommandFinished(CommandPtr command);
  
  CommandsContainer &_commandsContainer;
  vector<CommandPtr>::iterator _commandIterator;
  
  unsigned int _availableThreads;
  string _outputDir;
  unsigned int _threadsInUse;
  unsigned int _cumulatedTime;
};

#endif
