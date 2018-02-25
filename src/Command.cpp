

#include "Command.hpp"
#include "Common.hpp"
#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>


Command::Command(const string &id, 
    bool isMpiCommand,
    unsigned int ranks,
    unsigned int estimatedCost,
    const vector<string> &arguments):
  _id(id),
  _args(arguments),
  _isMpiCommand(isMpiCommand),
  _ranksNumber(ranks),
  _estimatedCost(estimatedCost)
{
}

string Command::toString() const 
{
  string res;
  res = getId() + " ";
  res += string(_isMpiCommand ? "mpi" : "nompi") + " ";
  for (auto str: _args) {
    res += str + " ";
  }
  res += "{ranks: " + to_string(getRanksNumber()) + ", estimated cost: "; 
  res += to_string(getEstimatedCost()) + "}";
  return res;
}
  
  
Instance::Instance(CommandPtr command,
    int startingRank,
    int ranksNumber):
  _command(command),
  _startingRank(startingRank),
  _ranksNumber(ranksNumber),
  _finished(false)
{
  
}

void Instance::onFinished()
{
  _finished = true;
  _endTime = Common::getTime();
}

static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

// Read a line in a commands file and skip comments
// discards empty lines
bool readNextLine(ifstream &is, string &os)
{
  while (getline(is, os)) {
    auto end = os.find("#");
    if (string::npos != end)
      os = os.substr(0, end);
    rtrim(os);
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
    
    string id;
    string isMPIStr;
    int ranks;
    int estimatedCost;
    
    istringstream iss(line);
    iss >> id;
    iss >> isMPIStr;
    iss >> ranks;
    iss >> estimatedCost;
    
    bool isMPI = (isMPIStr == string("mpi"));
    vector<string> commandVector;
    while (!iss.eof()) {
      string plop;
      iss >> plop;
      commandVector.push_back(plop);
    }
    CommandPtr command(new Command(id, isMPI, ranks, estimatedCost, commandVector));
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

CommandsRunner::CommandsRunner(const CommandsContainer &commandsContainer,
      shared_ptr<RanksAllocator> allocator,
      unsigned int availableRanks,
      const string &outputDir):
  _commandsContainer(commandsContainer),
  _availableRanks(availableRanks),
  _outputDir(outputDir),
  _allocator(allocator),
  _checkpoint(outputDir),
  _finishedInstancesNumber(0)
{
  cout << "The master process runs on node " << Common::getHost() 
       << " and on pid " << Common::getPid() << endl;
  for (auto command: commandsContainer.getCommands()) {
    if (!_checkpoint.isDone(command->getId())) {
      _commandsVector.push_back(command);
    }
  }
  sort(_commandsVector.begin(), _commandsVector.end(), compareCommands);
  _commandIterator = _commandsVector.begin();

}

void CommandsRunner::run() 
{
  cout << "Runner version with sleep 500ms" << endl;
  Timer globalTimer;
  Timer minuteTimer;
  int finishedInstancesNumber = 0;
  while (!_allocator->allRanksAvailable() || !isCommandsEmpty()) {
    if (minuteTimer.getElapsedMs() > 1000 * 60) {
      cout << "Runner is still alive after " << globalTimer.getElapsedMs() / 1000 << "s" << endl;
      minuteTimer.reset();
    } 
    if (!isCommandsEmpty()) {
      auto  nextCommand = getPendingCommand();
      if (_allocator->ranksAvailable()) {
        executePendingCommand();
        continue;
      }
    }
    vector<InstancePtr> finishedInstances = _allocator->checkFinishedInstances();
    for (auto instance: finishedInstances) {
      onFinishedInstance(instance);
    }
    Common::sleep(500);
  }
}


bool CommandsRunner::compareCommands(CommandPtr c1, CommandPtr c2)
{
  if (c2->getRanksNumber() == c1->getRanksNumber()) {
    return c2->getEstimatedCost() < c1->getEstimatedCost();
  }
  return c2->getRanksNumber() < c1->getRanksNumber();
}

  
void CommandsRunner::executePendingCommand()
{
  auto command = getPendingCommand();
  InstancePtr instance = _allocator->allocateRanks(command->getRanksNumber(), command);
  Timer t;
  Comment::printPidsNumber();
  instance->execute(instance);
  cout << "## Started " << command->getId() << " on [" 
    << instance->getStartingRank()  << ":"
    << instance->getStartingRank() + instance->getRanksNumber() - 1 
    << "] "
    << "(submit time " << t.getElapsedMs()  << "ms)" << endl;
  _historic.push_back(instance);
  _commandIterator++;
  if (isCommandsEmpty()) {
    cout << "All commands were launched" << endl;
  }
}

void CommandsRunner::onFinishedInstance(InstancePtr instance)
{
  instance->onFinished();
  _checkpoint.markDone(instance->getId());
  cout << "End of " << instance->getId() << " after " <<  instance->getElapsedMs() << "ms ";
  cout << " (" << ++_finishedInstancesNumber << "/" << _commandsVector.size() << ")" << endl;
 _allocator->freeRanks(instance);
}

RunStatistics::RunStatistics(const InstancesHistoric &historic,
    Time begin,
    Time end,
    int availableRanks):
  _historic(historic),
  _begin(begin),
  _end(end),
  _availableRanks(availableRanks)
{

}

void RunStatistics::printGeneralStatistics()
{
  int totalElapsedTime = Common::getElapsedMs(_begin, _end);
  int cumulatedTime = 0;
  int longestTime = 0;
  for (auto instance: _historic) {
    cumulatedTime += instance->getElapsedMs() * instance->getRanksNumber();
    longestTime = max(longestTime, instance->getElapsedMs());
  }
  double ratio = double(cumulatedTime) / double(_availableRanks * totalElapsedTime);
  
  cout << "Finished running commands. Total elasped time: ";
  cout << totalElapsedTime << "ms" << endl;
  cout << "The longest command took " << longestTime << "ms" << endl;
  cout << "Load balance ratio: " << ratio << endl;
}


void RunStatistics::exportSVG(const string &svgfile) 
{
  cout << "Saving svg output in " << svgfile << endl;
  int totalWidth = _availableRanks + 1;
  int totalHeight = Common::getElapsedMs(_begin, _end);
  double ratioWidth = 500.0 / double(totalWidth);
  double ratioHeight = 500.0 / double(totalHeight);
  SVGDrawer svg(svgfile, ratioWidth, ratioHeight);
  svg.writeSquare(0.0, 0.0, 1.0, 500.0 / ratioHeight, "#ffffff");
  for (auto instance: _historic) {
    instance->writeSVGStatistics(svg, _begin);
  }
}

