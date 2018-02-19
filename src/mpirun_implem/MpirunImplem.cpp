#include "MpirunImplem.hpp"
#include <iostream>

MpirunRanksAllocator::MpirunRanksAllocator(int availableRanks,
    const string &outputDir):
  _outputDir(outputDir)
{
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

void MpirunInstance::execute()
{
  cerr << "Not implemented" << endl;
}
  
void MpirunInstance::writeSVGStatistics(SVGDrawer &drawer, const Time &initialTime) 
{
  cerr << "Not implemented" << endl;
}

