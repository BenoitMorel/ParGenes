#include "Checkpoint.hpp"
#include "Common.hpp"
  
Checkpoint::Checkpoint(const string &outputDir)
{
  ifstream is(getCheckpointCommandsFile(outputDir));
  if (is) {
    while (!is.eof()) {
      string str;
      is >> str;
      if (!str.size()) {
        continue;
      }
      cout << "checkpoint detected <" << str << ">" << endl;
      _ids.insert(str);
    } 
  }
  _os = ofstream(getCheckpointCommandsFile(outputDir), std::fstream::app);
}

bool Checkpoint::isDone(const string &id)
{
  return _ids.find(id) != _ids.end();
}

void Checkpoint::markDone(const string &id)
{
  _os << id << endl;
}
  
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

string Checkpoint::getCheckpointCommandsFile(const string &outputDir)
{
  return Common::joinPaths(outputDir, "checkpoint_commands.txt");
}

