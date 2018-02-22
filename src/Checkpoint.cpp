#include "Checkpoint.hpp"
#include "Common.hpp"
#include <fstream>

  
string Checkpoint::readCheckpointArgs(const string &outputDir)
{
  ifstream is(getCheckpointArgsFile(outputDir));
  string res;
  while (!is.eof()) {
    string str;
    is >> str;
    res += str + " ";
  }
  return res;
}

void Checkpoint::writeCheckpointArgs(int argc, 
    char **argv,
    const string &outputDir)
{
  ofstream os(getCheckpointArgsFile(outputDir));
  os << Common::getSelfpath();
  for (int i = 1; i < argc; ++i) {
    os << " " << argv[i];
  }
}
  

string Checkpoint::getCheckpointArgsFile(const string &outputDir)
{
  return Common::joinPaths(outputDir, "checkpoint_args.txt");
}

