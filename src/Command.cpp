

#include "Command.hpp"
#include "Common.hpp"
#include <iostream>
#include <fstream>

Command::Command(const string &id, const string &command):
  _id(id),
  _command(command),
  _ranksNumber(1)
{

}

string Command::toString() const 
{
  string res;
  res = getId() + " " + getCommand();
  res += " {ranks: " + to_string(getRanksNumber()) + "}";
  return res;
}

// Read a line in a commands file and skip comments
// discards empty lines
bool readNextLine(std::ifstream &is, std::string &os)
{
  while (std::getline(is, os)) {
    auto end = os.find("#");
    if (std::string::npos != end)
      os = os.substr(0, end);
    if (os.size()) 
      return true;
  }
  return false;
}

CommandManager::CommandManager(const string &commandsFilename)
{
  ifstream reader(commandsFilename);
  if (!reader)
    throw MultiRaxmlException("Cannot open commands file ", commandsFilename);
  
  string line;
  while (readNextLine(reader, line)) {
    auto firstSpace = line.find(" ");
    if (string::npos == firstSpace) 
      throw MultiRaxmlException("Invalid syntax in commands file ", commandsFilename);
    CommandPtr command(new Command(line.substr(0, firstSpace), line.substr(firstSpace + 1)));
    addCommand(command);
    cout << command->toString() << endl;
  }
}

void CommandManager::addCommand(CommandPtr command)
{
  _commands.push_back(command); 
  _dicoCommands[command->getId()] = command;
}

CommandPtr CommandManager::getCommand(string id) const
{
  return _dicoCommands.at(id);
}
