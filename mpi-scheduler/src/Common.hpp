#ifndef _MULTIRAXML_COMMON_HPP_
#define _MULTIRAXML_COMMON_HPP_

#include <fstream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <mpi.h>
#include <signal.h>  

using namespace std;

namespace MPIScheduler {

class MPISchedulerException: public exception {
public:
  MPISchedulerException(const string &s): msg_(s) {}
  MPISchedulerException(const string &s1, 
      const string s2): msg_(s1 + s2) {}
  virtual const char* what() const throw() { return msg_.c_str(); }
  void append(const string &str) {msg_ += str;}

private:
  string msg_;
};

using Time = chrono::time_point<chrono::system_clock>;

class Common {
public:
  static void sleep(unsigned int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
  }

  // todobenoit not portable
  static void readDirectory(const std::string& name, 
      vector<string> &v)
  {
    DIR* dirp = opendir(name.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
      if (dp->d_name[0] != '.') {
        v.push_back(dp->d_name);
      }
    }
    closedir(dirp);
  }

  // todobenoit not portable
  static void makedir(const std::string &name) {
    mkdir(name.c_str(), 0755);
  }
  
  static string joinPaths(const std::string &path1, const std::string &path2) {
    return path1 + "/" + path2;
  }

  static string joinPaths(const std::string &path1, 
      const std::string &path2,
      const std::string &path3) {
    return joinPaths(joinPaths(path1, path2), path3);
  }

  static string getParentPath(const std::string &path) {
    return path.substr(0, path.find_last_of("/\\"));
  }

  static void removefile(const std::string &name) {
    remove(name.c_str());
  }

  static void removedir(const std::string &name);

  static Time getTime() {
    return chrono::system_clock::now();
  }

  static long getElapsedMs(Time begin, Time end) {
    return chrono::duration_cast<chrono::milliseconds>
      (end-begin).count();
  }

  // todobenoit not portable
  static string getSelfpath() {
    char buff[1000];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    buff[len] = '\0';
    return string(buff);
  }

  static void check(int mpiError) {
    if (mpiError != MPI_SUCCESS) {
      throw MPISchedulerException("MPI error !");
    }
  }

  static int getPid() {
    return getpid();
  }
  
  static bool isPidAlive(int pid) {
    return 0 == kill(pid, 0);  
  }
  static string getHost() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return string(hostname);
  }

  static string getProcessIdentifier() {
    return getHost() + "_" + to_string(getPid()); 
  }

  static string getHostfilePath(const string &outputDir) {
    return joinPaths(outputDir, "hostfile");
  }

  static void printPidsNumber()
  {
    system("ps -eaf |  wc -l | awk '{print \"pids \"$1}'");
  }
};


class Timer {
public:
  Timer() {
    reset();
  }

  int getElapsedMs() const {
    auto end = Common::getTime();
    return Common::getElapsedMs(_start, end);
  }

  void reset() {
    _start = chrono::system_clock::now();
  }
private:
  Time _start;
};

class SVGDrawer {
public:
  SVGDrawer(const string &filepath,
      double maxXValue,
      double maxYValue);
  ~SVGDrawer();
  void writeSquare(double x, double y, double w, double h, const char *color = 0);
  void writeSquareAbsolute(double x, double y, double w, double h, const char *color = 0);
  void writeCaption(const string &text);
  void writeHorizontalLine(double y, int lineWidth); 
  static string getRandomHex();

private:
  void writeHeader();
  void writeFooter();
  ofstream _os;
  double _width;
  double _height;
  double _ratioWidth;
  double _ratioHeight;
  double _additionalHeight;
};

class SchedulerArgumentsParser {
public:
  SchedulerArgumentsParser(int argc, char** argv):
    commandsFilename()
  {
    if (argc != 5) {
      print_help();
      throw MPISchedulerException("Error: invalid syntax");
    }
    unsigned int i = 2;
    library = string(argv[i++]);
    commandsFilename = string(argv[i++]);
    outputDir = string(argv[i++]);
  }
  
  void print_help() 
  {
    cout << "Syntax:" << endl;
    cout << "multiraxml --split-scheduler library command_file output_dir" << endl;
  }

  string library;
  string commandsFilename;
  string outputDir;
};

} // namespace MPIScheduler

#endif
