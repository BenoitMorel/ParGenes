#ifndef _MULTIRAXML_CHECKPOINT_HPP_
#define _MULTIRAXML_CHECKPOINT_HPP_

#include <string>
#include <fstream>
#include <set>

using namespace std;

namespace MPIScheduler {

class Checkpoint {
public:
  explicit Checkpoint(const string &outputDir); 
 
  bool isDone(const string &id);
  void markDone(const string &id);

private:
  static string getCheckpointCommandsFile(const string &outputDir);

private:
  set<string> _ids; 
  ofstream _os;
};

} // namespace MPIScheduler

#endif

