# Multi-raxml 

Run a high number of raxml instances on a given number of cores.


## Requirement

* git
* cmake
* MPI
* python 3


## Installation

Clone and build Multi-raxml:

```
git clone https://github.com/BenoitMorel/multi-raxml.git
cd multi-raxml
mkdir build
cd build
cmake ..
```


Clone and build raxml:
In another directory
```
git clone https://github.com/amkozlov/raxml-ng.git
cd raxml-ng
mkdir build
cmake -DBUILD_AS_LIBRARY=ON -DUSE_MPI=ON ..
make
```


## Run

See wiki

