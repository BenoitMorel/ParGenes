#include <mpi.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char** argv) 
{
  // MPI_Init(&argc, &argv);
  string str;
  for (unsigned int i = 2; i < argc; ++i) {
    str += string(argv[i]) + " ";
  }
  str += " > /dev/null ";
  system(str.c_str());
  //MPI_Finalize();
  ofstream out(argv[1]);
  out.close();
  return 0;
}

