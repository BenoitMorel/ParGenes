

#include "Command.hpp"
#include "Common.hpp"
#include "SVGDrawer.hpp"
#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>

namespace MPIScheduler {

Command::Command(const string &id, 
    unsigned int ranks,
    long estimatedCost,
    const vector<string> &arguments):
  _id(id),
  _args(arguments),
  _ranksNumber(ranks),
  _estimatedCost(estimatedCost)
{
}

Instance::Instance(CommandPtr command,
    int startingRank,
    int ranksNumber,
    const string &outputDir):
  _startingElapsedMS(0),
  _outputDir(outputDir),
  _command(command),
  _startingRank(startingRank),
  _ranksNumber(ranksNumber),
  _beginTime(Common::getTime()),
  _endTime(Common::getTime()),
  _elapsed(0)
{
}

void Instance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime)
{
  drawer.writeSquare(getStartingRank(),
    _startingElapsedMS,
    getRanksNumber(),
    getElapsedMs());

}
void Instance::onFinished()
{
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
    throw MPISchedulerException("Cannot open commands file ", commandsFilename);
  
  string line;
  while (readNextLine(reader, line)) {
    
    string id;
    int ranks;
    long estimatedCost;
    
    istringstream iss(line);
    iss >> id;
    iss >> ranks;
    iss >> estimatedCost;
    
    if (ranks == 0) {
      cerr << "[mpi-scheduler warning] Found a job with 0 ranks... will assign 1 rank instead" << endl;
      ranks = 1;
    }

    vector<string> commandVector;
    commandVector.push_back("multi-raxml");
    while (!iss.eof()) {
      string argument;
      iss >> argument;
      commandVector.push_back(argument);
    }
    CommandPtr command(new Command(id, ranks, estimatedCost, commandVector));
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
      const string &outputDir):
  _commandsContainer(commandsContainer),
  _outputDir(outputDir),
  _allocator(allocator),
  _checkpoint(outputDir),
  _finishedInstancesNumber(0),
  _verbose(true)
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
  cout << "Remaining commands: " << _commandsVector.size() << endl;

}

void CommandsRunner::run() 
{
  Timer globalTimer;
  Timer minuteTimer;
  while (!_allocator->allRanksAvailable() || !isCommandsEmpty()) {
    if (minuteTimer.getElapsedMs() > 1000 * 60) {
      cout << "Runner is still alive after " << globalTimer.getElapsedMs() / 1000 << "s" << endl;
      minuteTimer.reset();
    } 
    if (!isCommandsEmpty()) {
      if (_allocator->ranksAvailable()) {
        executePendingCommand();
      }
    }
    vector<InstancePtr> finishedInstances = _allocator->checkFinishedInstances();
    for (auto instance: finishedInstances) {
      onFinishedInstance(instance);
    }
  }
}
  
void CommandsRunner::runOpenMP()
{
  // pragma for
  for (int i = 0; i < _commandsContainer.getCommands().size(); ++i) {
    auto command = _commandsContainer.getCommands()[i];
    auto instance = _allocator->allocateRanks(1, command); 
    if (!instance->execute(instance)) {
      cout << "Failed to start command " << command->getId() << endl;
      continue;
    }
    // critical
    _historic.push_back(instance);
    onFinishedInstance(instance);
  }

}


bool CommandsRunner::compareCommands(CommandPtr c1, CommandPtr c2)
{
  if (c2->getRanksNumber() == c1->getRanksNumber()) {
    return c2->getEstimatedCost() < c1->getEstimatedCost();
  }
  return c2->getRanksNumber() < c1->getRanksNumber();
}

  
bool CommandsRunner::executePendingCommand()
{
  auto command = getPendingCommand();
  InstancePtr instance = _allocator->allocateRanks(command->getRanksNumber(), command);
  if (!instance->execute(instance)) {
    cout << "Failed to start " << command->getId() << ". Will retry later " << endl;
    _allocator->freeRanks(instance);
    return false;
  }
    
  if (_verbose) {
    cout << "## Started " << command->getId() << " on [" 
      << instance->getStartingRank()  << ":"
      << instance->getStartingRank() + instance->getRanksNumber() - 1 
      << "] "
      << endl;
  }
  _historic.push_back(instance);
  ++_commandIterator;
  if (isCommandsEmpty()) {
    cout << "All commands were launched" << endl;
  }
  return true;
}

void CommandsRunner::onFinishedInstance(InstancePtr instance)
{
  instance->onFinished();
  _checkpoint.markDone(instance->getId());
  if (_verbose) {
    cout << "End of " << instance->getId() << " after " <<  instance->getElapsedMs() << "ms ";
    cout << " (" << ++_finishedInstancesNumber << "/" << _commandsVector.size() << ")" << endl;
  }
 _allocator->freeRanks(instance);
}

RunStatistics::RunStatistics(const InstancesHistoric &historic,
    Time begin,
    Time end,
    int availableRanks):
  _historic(historic),
  _begin(begin),
  _end(end),
  _availableRanks(availableRanks),
  _lbRatio(1.0)
{

}

void RunStatistics::printGeneralStatistics()
{
  long totalElapsedTime = Common::getElapsedMs(_begin, _end);
  long cumulatedTime = 0;
  for (auto instance: _historic) {
    cumulatedTime += instance->getElapsedMs() * instance->getRanksNumber();
  }
  _lbRatio = double(cumulatedTime) / double(_availableRanks * totalElapsedTime);
  
  cout << "Finished running commands. Total elasped time: ";
  cout << totalElapsedTime / 1000  << "s" << endl;
  cout << "Load balance ratio: " << _lbRatio << endl;
}


void RunStatistics::exportSVG(const string &svgfile) 
{
  Timer t;
  cout << "Saving svg output in " << svgfile << endl;
  int totalWidth = _availableRanks + 1;
  int totalHeight = Common::getElapsedMs(_begin, _end);
  string caption = "t = " + to_string(totalHeight / 1000) + "s";
  caption += ", lb = " + to_string(_lbRatio);
  SVGDrawer svg(svgfile, double(totalWidth), double(totalHeight));
  svg.writeHeader(caption);
  for (auto instance: _historic) {
    instance->writeSVGStatistics(svg, _begin);
  }
  svg.writeFooter();
  cout << "Time spent writting svg: " << t.getElapsedMs() / 1000 << "s" << endl;
}

} // namespace MPIScheduler

