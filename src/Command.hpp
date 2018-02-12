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

class CommandManager {
public:
  CommandManager(const string &commandsFilename,
      unsigned int availableThreads,
      const string &outputDir);
  
  void addCommand(CommandPtr command);
  CommandPtr getCommand(string id) const;
  const string &getOutputDir() const {return _outputDir;}

  void run();
private:
  void checkCommandsFinished();

  vector<CommandPtr> _commands;
  map<string, CommandPtr> _dicoCommands;
  unsigned int _availableThreads;
  string _outputDir;
  unsigned int _threadsInUse;
  unsigned int _cumulatedTime;
};


#endif
