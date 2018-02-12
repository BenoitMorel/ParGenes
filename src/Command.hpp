#ifndef _MULTIRAXML_COMMAND_HPP_
#define _MULTIRAXML_COMMAND_HPP_

#include <string>
#include <memory>
#include <vector>
#include <map>

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
public:
  string _id;
  string _command;
  int _ranksNumber;
};

using CommandPtr = shared_ptr<Command>;

class CommandManager {
public:
  CommandManager(const string &commandsFilename);
  void addCommand(CommandPtr command);
  CommandPtr getCommand(string id) const;
private:
  vector<CommandPtr> _commands;
  map<string, CommandPtr> _dicoCommands;
};


#endif
