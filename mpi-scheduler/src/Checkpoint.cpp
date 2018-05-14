#include "Checkpoint.hpp"
#include "Common.hpp"

namespace MPIScheduler {

Checkpoint::Checkpoint(const string &outputDir):
  _os(getCheckpointCommandsFile(outputDir), std::fstream::app)
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
}

bool Checkpoint::isDone(const string &id)
{
  return _ids.find(id) != _ids.end();
}

void Checkpoint::markDone(const string &id)
{
  _os << id << endl;
}
  
string Checkpoint::getCheckpointCommandsFile(const string &outputDir)
{
  return Common::joinPaths(outputDir, "checkpoint_commands.txt");
}

} // namespace MPIScheduler

