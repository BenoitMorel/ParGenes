#include "SpawnImplem.hpp"
#include <mpi.h>
#include <iostream>

// spawned from SpawnInstance::execute
void main_spawned_wrapper(int argc, char** argv) 
{
  Timer t;
  ifstream argumentsReader(argv[2]);
  string id, outputDir;
  int isMPI;
  argumentsReader >> id;
  argumentsReader >> outputDir;
  argumentsReader >> isMPI;

  
  ofstream osstarted(Common::joinPaths(outputDir, "started", id + "_" + Common::getProcessIdentifier()));
  osstarted.close();

  // if the program to spawn does not call MPI_Init
  // we have to do it here
  if (!isMPI) { 
    Common::check(MPI_Init(&argc, &argv));
  }

  // build and run the command
  string command;
  while (!argumentsReader.eof()) {
    string tmp;
    argumentsReader >> tmp;
    command += tmp + " ";
    
  }
  command += " > " +  Common::joinPaths(outputDir, "per_job_logs", id)  + ".spawned.out 2>&1 ";
  try {
    system(command.c_str());
  } catch(...) {
    cerr << "FAILUUUUUURE" << endl;
    ofstream out(Common::joinPaths(outputDir, "per_job_logs", id) + ".failure");
    out << ("Command " + id + " failed");
  } 
  if (!isMPI) {
    Common::check(MPI_Finalize());
  }

  // write a file to signal the master process that I am  done
  // (be carefull, several processes like me are called with the same id)
  string tempid = Common::joinPaths(outputDir, "temp", id);
  Common::makedir(tempid);
  string name = Common::joinPaths(tempid, Common::getProcessIdentifier());
  ofstream out(name);
  out << t.getElapsedMs();
  out.close();
}


SpawnedRanksAllocator::SpawnedRanksAllocator(int availableRanks,
    const string &outputDir):
  _ranksInUse(0),
  _outputDir(outputDir)
{
  _slots.push(std::pair<int,int>(1, availableRanks));
  Common::makedir(Common::joinPaths(outputDir, "temp"));
  Common::makedir(Common::joinPaths(outputDir, "per_job_logs"));
  Common::makedir(Common::joinPaths(outputDir, "started"));
  Common::makedir(Common::joinPaths(outputDir, "orders"));
}


bool SpawnedRanksAllocator::ranksAvailable()
{
  return !_slots.empty();
}
 
bool SpawnedRanksAllocator::allRanksAvailable()
{
  return _ranksInUse == 0;
}
  
InstancePtr SpawnedRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  std::pair<int, int> result = _slots.top();
  _slots.pop();
  int startingRank = result.first;
  int threadsNumber = requestedRanks; 
  // border case around the first rank
  if (result.first == 1 && requestedRanks != 1) {
    threadsNumber -= 1; 
  }
  if (result.second > threadsNumber) {
    result.first += threadsNumber;
    result.second -= threadsNumber;
    _slots.push(result);
  }
  if (result.second < threadsNumber) {
    threadsNumber = result.second;
  }
  _ranksInUse += threadsNumber;
  InstancePtr instance(new SpawnInstance(_outputDir,
    startingRank,
    threadsNumber,
    command));
  if (_startedInstances.find(instance->getId()) != _startedInstances.end()) {
    cerr << "Warning: two instances have the same id" << endl;
  }
  _startedInstances[instance->getId()] = instance;
  return instance;
}
  
void SpawnedRanksAllocator::freeRanks(InstancePtr instance)
{
  _ranksInUse -= instance->getRanksNumber();
  _slots.push(std::pair<int,int>(instance->getStartingRank(), 
        instance->getRanksNumber()));
}

vector<InstancePtr> SpawnedRanksAllocator::checkFinishedInstances()
{
  vector<string> files;
  string temp = Common::joinPaths(_outputDir, "temp");
  Common::readDirectory(temp, files);
  vector<InstancePtr> finished;
  for (auto file: files) {
    auto instance = _startedInstances.find(file);
    if (instance != _startedInstances.end()) {
      string fullpath = Common::joinPaths(temp, instance->second->getId());
      vector<string> subfiles;
      Common::readDirectory(fullpath, subfiles);
      if (subfiles.size() != instance->second->getRanksNumber()) {
        continue;
      }
      ifstream timerFile(Common::joinPaths(fullpath, subfiles[0]));
      int realElapsedTimeMS = 0;
      timerFile >> realElapsedTimeMS;
      timerFile.close();
      instance->second->setElapsedMs(realElapsedTimeMS);
      Common::removedir(fullpath);
      finished.push_back(instance->second);
    }
  }
  return finished;
}


SpawnInstance::SpawnInstance(const string &outputDir, 
  int startingRank, 
  int ranksNumber,
  CommandPtr command):
  Instance(command, startingRank, ranksNumber),
  _outputDir(outputDir)
{
}


void SpawnInstance::execute(InstancePtr self)
{
  _beginTime = Common::getTime();
  _finished = false;
  if (_ranksNumber == 0) {
    throw MultiRaxmlException("Error in SpawnInstance::execute: invalid number of ranks ", to_string(_ranksNumber));
  }

  string argumentFilename = Common::joinPaths(_outputDir, "orders", getId());
  ofstream argumentFile(argumentFilename);
  argumentFile << getId() << " " << _outputDir << " " << _command->isMpiCommand() << endl;
  for(auto arg: _command->getArgs()) {
    argumentFile << arg << " ";
  }
  argumentFile.close(); 
  
  static string spawnedArg = "--spawned-wrapper";
  char *argv[3];
  argv[0] = (char *)spawnedArg.c_str();
  argv[1] = (char *)argumentFilename.c_str();
  argv[2] = 0;

  string wrapperExec = Common::getSelfpath();
  MPI_Comm intercomm;
  try {
    Common::check(MPI_Comm_spawn((char*)wrapperExec.c_str(), 
          argv, getRanksNumber(),  
          MPI_INFO_NULL, 0, MPI_COMM_SELF, &intercomm,  
          MPI_ERRCODES_IGNORE));
  } catch (...) {
    cerr << "SOMETHING FAIILED" << endl;
  }
}
  
void SpawnInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  drawer.writeSquare(getStartingRank(),
    Common::getElapsedMs(initialTime, getStartTime()),
    getRanksNumber(),
    getElapsedMs());
}
