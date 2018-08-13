#include "ParallelImplementation.hpp"

#include <assert.h>

#ifdef WITH_MPI
#include "split_implem/SplitImplem.hpp"
#include "onecore_implem/OneCoreImplem.hpp"
#include <mpi.h>
#endif
#ifdef WITH_OPENMP
#include "openmp_implem/OpenMPImpl.hpp"
#endif

#include "Command.hpp"
#include "Common.hpp"


namespace MPIScheduler {

ParallelImplementation::ParallelImplementation(const string implem): _rank(0), _ranksNumber(0) {
  if (implem == "--split-scheduler")
    _impl = split;
  else if (implem == "--onecore-scheduler") 
    _impl = onecore;
  else if (implem == "--openmp-scheduler")
    _impl = openmp;
  else
    _impl = invalid;
}

bool ParallelImplementation::isValid() const {
#ifndef WITH_MPI
  if (isMPI()) { 
    cerr << "Error: trying to use an MPI implementation that was not compiled" << endl;
    return false;
  }
#endif
#ifndef WITH_OPENMP
  if (isOpenMP()) { 
    cerr << "Error: trying to use an OpenMP implementation that was not compiled" << endl;
    return false;
  }
#endif
  return _impl != invalid;
}

int ParallelImplementation::getRank() const {return _rank;}
int ParallelImplementation::getRanksNumber() const {return _ranksNumber;}

bool ParallelImplementation::isMPI() const {
  return (_impl == split) || (_impl == onecore); 
}

bool ParallelImplementation::isOpenMP() const {
  return _impl == openmp;
}

void ParallelImplementation::initParallelContext(int argc, char **argv) {
  if (isMPI()) {
#ifdef WITH_MPI
    Common::check(MPI_Init(&argc, &argv));
    MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &_ranksNumber);
#else
    assert(0);
#endif
  } else if (isOpenMP()) {
#ifdef WITH_OPENMP
    _rank = 0;
    _ranksNumber = getOpenMPThreads();
#endif
  } else {
    _rank = 0;
    _ranksNumber = 0;
  }
}

void ParallelImplementation::closeParallelContext() {
  if (isMPI()) {
#ifdef WITH_MPI
    MPI_Finalize();
#else
    assert(0);
#endif
  }
}

bool ParallelImplementation::slavesToStart() const {
  return isMPI();
}

void ParallelImplementation::startSlaves(int argc, char **argv) {
  if (getRank() != getRanksNumber() - 1) {
#ifdef WITH_MPI
    if (_impl == split) {
      SplitSlave slave;
      slave.main_split_slave(argc, argv);
    } else if (_impl = onecore) {
      OneCoreSlave slave;
      slave.main_core_slave(argc, argv);
    }
#endif
    exit(0);
  }
}

shared_ptr<RanksAllocator> ParallelImplementation::getRanksAllocator(SchedulerArgumentsParser &arg,
                                  int ranksNumber) {
#ifdef WITH_MPI
  if (_impl == split) {
    return shared_ptr<RanksAllocator>(new SplitRanksAllocator(ranksNumber,
        arg.outputDir));
  } else if (_impl == onecore) {
    return shared_ptr<RanksAllocator>(new OneCoreRanksAllocator(ranksNumber,
        arg.outputDir));
  }
#endif
#ifdef WITH_OPENMP
  if (_impl == openmp) {
    return shared_ptr<RanksAllocator>(new OpenMPRanksAllocator(arg.outputDir, arg.library));
  }
#endif
  assert(0);
  return shared_ptr<RanksAllocator>(0);
}

}


