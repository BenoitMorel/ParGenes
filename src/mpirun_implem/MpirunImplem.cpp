#include "MpirunImplem.hpp"
#include <iostream>
#include <mpi.h>

using namespace std;

void remove_domain(string &str)
{
  str = str.substr(0, str.find(".", 0));
}

void main_mpirun_hostfile(int argc, char** argv)
{
  MPI_Init(NULL, NULL);
  
  int name_len;
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(processor_name, &name_len);
  
  char *recieve_data = new char[MPI_MAX_PROCESSOR_NAME * world_size];

  MPI_Gather(processor_name,
      MPI_MAX_PROCESSOR_NAME,
      MPI_CHAR,
      recieve_data,
      MPI_MAX_PROCESSOR_NAME,
      MPI_CHAR,
      0,
      MPI_COMM_WORLD);
  if (!rank) {
    map<string, int> nodes;;
    for (unsigned int i = 0; i < world_size; ++i) {
      string node(recieve_data + i * MPI_MAX_PROCESSOR_NAME);
      remove_domain(node); // todobenoit this might be wrong
      int size = 0;
      if (nodes.find(node) != nodes.end()) {
        size = nodes.at(node);
      }
      nodes[node] = ++size;
    }
    for (auto pair: nodes) {
      cout << pair.first << " slots = " << pair.second << endl;
    }
  }
  delete[] recieve_data;
  MPI_Finalize();
}





MpirunRanksAllocator::MpirunRanksAllocator(int availableRanks,
    const string &outputDir):
  _outputDir(outputDir),
  _ranks(availableRanks)
{
  computePinning();
}


bool MpirunRanksAllocator::ranksAvailable()
{
  cerr << "Not implemented" << endl;
  return false;
}
 
bool MpirunRanksAllocator::allRanksAvailable()
{
  cerr << "Not implemented" << endl;
  return false;
}
  
InstancePtr MpirunRanksAllocator::allocateRanks(int requestedRanks, 
  CommandPtr command)
{
  cerr << "Not implemented" << endl;
  return 0;
}
  
void MpirunRanksAllocator::freeRanks(InstancePtr instance)
{
  cerr << "Not implemented" << endl;
}

vector<InstancePtr> MpirunRanksAllocator::checkFinishedInstances()
{
  cerr << "Not implemented" << endl;
  vector<InstancePtr> finished;
  return finished;
}

void MpirunRanksAllocator::computePinning() 
{
  string hostfilePath = Common::joinPaths(_outputDir, "hostfile");
  string commandPrinters = string("mpirun -np ")
    + to_string(_ranks) + string(" ") + Common::getSelfpath()
    + string ("--mpirun-hostfile ") + string("> ") + hostfilePath;
  system(commandPrinters.c_str());
  ifstream hostfileReader(hostfilePath);
  vector<pair<string, int> > nodes;
  int currRank  = 0;
  while (!hostfileReader.eof()) {
    string host, temp;
    int slots;
    hostfileReader >> host;
    if (hostfileReader.eof())
      break;
    hostfileReader >> temp;
    hostfileReader >> temp;
    hostfileReader >> slots;
    for (int i = 0; i < slots; ++i) {
      cout << "assign rank " << currRank << " to host " << host << endl;
      _ranksToPinning[currRank] = Pinning(currRank, host);
      currRank++;
    }
  }
}
  
MpirunInstance::MpirunInstance(CommandPtr command,
  int startingRank, // todobenoit 
  int ranksNumber, 
  const string &outputDir,
  const Pinnings &pinnings):
  _outputDir(outputDir),
  _pinnings(pinnings),
  Instance(command, startingRank, ranksNumber)
{
}

void MpirunInstance::execute()
{
  cout << "Executing command " << getId() 
       << " on nodes " << endl;
  for (auto pin: _pinnings) {
    cout << "  " <<  pin.node << endl;
  }
}
  
void MpirunInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  cerr << "Not implemented" << endl;
}

