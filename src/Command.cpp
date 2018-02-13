

#include "Command.hpp"
#include "Common.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mpi.h>
#include <iterator>
#include <algorithm>

const string python = "python";
const string pythonUseCommand = "-c";
const string pythonCommand = 
"import sys\n"
"import subprocess\n"
"import os\n"
"commands = sys.argv[2:]\n"
"FNULL = open(os.devnull, 'w')\n"
"subprocess.check_call(commands, stdout=FNULL, stderr=FNULL)\n"
"f = open(sys.argv[1], \"w\")\n";


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
  
void Command::execute(const string &outputDir)
{
  _beginTime = Common::getTime();
  istringstream iss(getCommand());
  vector<string> splitCommand(istream_iterator<string>{iss},
                                       istream_iterator<string>());
  char **argv = new char*[splitCommand.size() + 3];
  string infoFile = outputDir + "/" + getId(); // todobenoit not portable
  argv[0] = (char*)pythonUseCommand.c_str();
  argv[1] = (char*)pythonCommand.c_str();
  argv[2] = (char*)infoFile.c_str();
  unsigned int offset = 3;
  for(unsigned int i = 0; i < splitCommand.size(); ++i)
    argv[i + offset] = (char*)splitCommand[i].c_str();
  argv[splitCommand.size() + offset] = 0;

  MPI_Comm intercomm;
  MPI_Comm_spawn((char*)python.c_str(), argv, getRanksNumber(),  
          MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomm,  
          MPI_ERRCODES_IGNORE);
  delete[] argv;
}
  
void Command::onFinished()
{
  _endTime = Common::getTime();
}

// Read a line in a commands file and skip comments
// discards empty lines
bool readNextLine(ifstream &is, string &os)
{
  while (getline(is, os)) {
    auto end = os.find("#");
    if (string::npos != end)
      os = os.substr(0, end);
    if (os.size()) 
      return true;
  }
  return false;
}

CommandsContainer::CommandsContainer(const string &commandsFilename)
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
  }
}

void CommandsContainer::addCommand(CommandPtr command)
{
  _commands.push_back(command); 
  _dicoCommands[command->getId()] = command;
}

CommandPtr CommandsContainer::getCommand(string id) const
{
  auto res =  _dicoCommands.find(id);
  if (res == _dicoCommands.end())
    return CommandPtr(0);
  else 
    return res->second;
}


CommandsRunner::CommandsRunner(CommandsContainer &commandsContainer,
      unsigned int availableThreads,
      const string &outputDir):
  _commandsContainer(commandsContainer),
  _commandIterator(commandsContainer.getCommands().begin()),
  _availableThreads(availableThreads),
  _outputDir(outputDir),
  _threadsInUse(0)
{

}

void CommandsRunner::run() 
{
  Timer timer;
  unsigned int nextCommandIndex = 0;
  _threadsInUse = 0;
  while (_threadsInUse || !isCommandsEmpty()) {
    if (!isCommandsEmpty()) {
      auto  nextCommand = getPendingCommand();
      if (_threadsInUse + nextCommand->getRanksNumber() <= _availableThreads) {
        executePendingCommand();
        continue;
      }
    }
    checkCommandsFinished();
    Common::sleep(100);
  }
}
  
void CommandsRunner::executePendingCommand()
{
  auto command = getPendingCommand();
  cout << "Executing command " << command->toString() << endl;
  command->execute(getOutputDir());
  _threadsInUse += command->getRanksNumber();
  _commandIterator++;
}

void CommandsRunner::checkCommandsFinished()
{
  vector<string> files;
  Common::readDirectory(_outputDir, files);
  for (auto file: files) {
    CommandPtr command = _commandsContainer.getCommand(file);
    if (command) {
      onCommandFinished(command);
    }
  }
}

void CommandsRunner::onCommandFinished(CommandPtr command)
{
  command->onFinished();
  string fullpath = _outputDir + "/" + command->getId(); // todobenoit not portable
  Common::removefile(fullpath);
  _threadsInUse -= command->getRanksNumber();
  cout << "Command " << command->getId() << " finished after ";
  cout << command->getElapsedMs() << "ms" << endl;
}

CommandsStatistics::CommandsStatistics(const CommandsContainer &commands,
    Time begin,
    Time end,
    int availableThreads):
  _commands(commands),
  _begin(begin),
  _end(end),
  _availableThreads(availableThreads)
{

}

void CommandsStatistics::printGeneralStatistics()
{
  int totalElapsedTime = Common::getElapsedMs(_begin, _end);
  int cumulatedTime = 0;
  int longestTime = 0;
  for (auto command: _commands.getCommands()) {
    cumulatedTime += command->getElapsedMs() * command->getRanksNumber();
    longestTime = max(longestTime, command->getElapsedMs());
  }
  double ratio = double(cumulatedTime) / double(_availableThreads * totalElapsedTime);
  
  cout << "Finished running commands. Total elasped time: ";
  cout << totalElapsedTime << "ms" << endl;
  cout << "The longest command took " << longestTime << "ms" << endl;
  cout << "Load balance ratio: " << ratio << endl;
}

void CommandsStatistics::exportSVG(const string &svgfile) 
{

}




