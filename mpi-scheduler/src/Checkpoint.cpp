#include "Checkpoint.hpp"
#include "Common.hpp"

namespace MPIScheduler {

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
  
string Checkpoint::getCheckpointCommandsFile(const string &outputDir)
{
  return Common::joinPaths(outputDir, "checkpoint_commands.txt");
}

} // namespace MPIScheduler

