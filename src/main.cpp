
#include <mpi.h>
#include <iostream>
#include <string>



void spawn(char *prefix)
{
  char * raxmlExec = "/home/morelbt/github/raxdog/thirdlib/raxml-ng/bin/raxml-ng-mpi";
  char *argv[7] = {"--msa", 
    "/home/morelbt/github/PHYLDOG/benoitdata/DataCarine/FastaFiles/ENSGT00440000039287.fa",
    "--model", 
    "GTR", 
    "--prefix",  
    prefix,
    0};
  MPI_Comm sonComm;
  std::cerr << "before spawn" << std::endl;
  MPI_Comm_spawn(raxmlExec,
      argv,
      15,
      MPI_INFO_NULL, 
      0, 
      MPI_COMM_WORLD,
      &sonComm,
      MPI_ERRCODES_IGNORE);
}

int main(int argc, char** argv) {
  MPI_Init(argc, argv);
  std::cout << "Plouf" << std::endl;

  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  
  std::cout << world_size << " " << world_rank << std::endl;

  spawn("/home/morelbt/github/multi-raxml/result/bidon_1");
  spawn("/home/morelbt/github/multi-raxml/result/bidon_2");

  std::cerr << "spawned" << std::endl;
  MPI_Finalize();

  std::cerr << "end" << std::endl;
  return 0;
}
