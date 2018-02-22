#include "Checkpoint.hpp"
#include "Common.hpp"
#include <fstream>

void Checkpoint::readCheckpointArgs(int &argc, 
    char **&argv,
    const string &outputDir)
{
  ifstream is(getCheckpointArgsFile(outputDir));
  is >> argc;
  argv = new char*[argc];
  for (int i = 0; i < argc; ++i) {
    string str;
    is >> str;
    argv[i] = new char[str.size() + 1];
    argv[i][str.size()] = 0;
    memcpy(argv[i], str.c_str(), str.size());
  }
}

void Checkpoint::writeCheckpointArgs(int argc, 
    char **argv,
    const string &outputDir)
{
  ofstream os(getCheckpointArgsFile(outputDir));
  os << argc;
  for (int i = 0; i < argc; ++i) {
    os << " " << argv[i] << " ";
  }
}
  

string Checkpoint::getCheckpointArgsFile(const string &outputDir)
{
  return Common::joinPaths(outputDir, "checkpoint_args.txt");
}

