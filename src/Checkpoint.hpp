#ifndef _MULTIRAXML_CHECKPOINT_HPP_
#define _MULTIRAXML_CHECKPOINT_HPP_

#include <string>

using namespace std;

class Checkpoint {
public:
  static string readCheckpointArgs(const string &outputDir);

  static void writeCheckpointArgs(int argc, 
      char **argv,
      const string &outputDir);

private:
  static string getCheckpointArgsFile(const string &outputDir);
};

#endif

