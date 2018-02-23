#ifndef _MULTIRAXML_CHECKPOINT_HPP_
#define _MULTIRAXML_CHECKPOINT_HPP_

#include <string>
#include <fstream>
#include <set>

using namespace std;

class Checkpoint {
public:
  Checkpoint(const string &outputDir); 
 
  bool isDone(const string &id);
  void markDone(const string &id);

public:
  static string readCheckpointArgs(const string &outputDir);

  static void writeCheckpointArgs(int argc, 
      char **argv,
      const string &outputDir);


private:
  static string getCheckpointArgsFile(const string &outputDir);
  static string getCheckpointCommandsFile(const string &outputDir);

private:
  set<string> _ids; 
  ofstream _os;
};

#endif

