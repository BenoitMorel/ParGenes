

#include "Command.hpp"
#include "Common.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mpi.h>
#include <iterator>

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
  cout << "Executing command " << toString() << endl;
  _timer.reset();
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
  _availableThreads(availableThreads),
  _outputDir(outputDir),
  _threadsInUse(0),
  _cumulatedTime(0)
{

}

void CommandsRunner::run() 
{
  vector<CommandPtr> commands = _commandsContainer.getCommands();
  Timer timer;
  unsigned int nextCommandIndex = 0;
  _threadsInUse = 0;
  while (_threadsInUse || nextCommandIndex < commands.size()) {
    if (nextCommandIndex < commands.size()) {
      auto  nextCommand = commands[nextCommandIndex];
      if (_threadsInUse + nextCommand->getRanksNumber() <= _availableThreads) {
        _threadsInUse += nextCommand->getRanksNumber();
        nextCommand->execute(getOutputDir());
        nextCommandIndex++;
        continue;
      }
    }
    checkCommandsFinished();
    MultiRaxmlCommon::sleep(100);
  }
  cout << "Finished running commands. Total elasped time: " 
    << timer.getElapsedMs() << "ms" << endl;
  double totalElapsed = double(timer.getElapsedMs());
  double totalCumulated = double(_cumulatedTime) / double(_availableThreads);
  cout << "Load balance ratio: " << totalCumulated / totalElapsed << endl;
}

void CommandsRunner::checkCommandsFinished()
{
  vector<string> files;
  MultiRaxmlCommon::readDirectory(_outputDir, files);
  for (auto file: files) {
    CommandPtr command = _commandsContainer.getCommand(file);
    if (command) {
      string fullpath = _outputDir + "/" + file; // todobenoit not portable
      MultiRaxmlCommon::removefile(fullpath);
      _threadsInUse -= command->getRanksNumber();
      cout << "Command " << file << " finished after ";
      cout << command->getElapsedMs() << "ms" << endl;
      _cumulatedTime += command->getElapsedMs();
    }
  }
}


