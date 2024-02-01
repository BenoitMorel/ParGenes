# ParGenes

A massively parallel tool for model selection and tree inference on thousands of genes

## Features

ParGenes is a parallel tool that takes as input a set of multiple sequence alignments (typically from different genes) and infers their corresponding phylogenetic trees.

ParGenes supports the following features:

* best-fit model selection with ModelTest
* phylogenetic tree inference with RAxML-NG (with customizable number of starting trees and bootstrap trees)
* automatic parallelization to make the best use of the available computing cores
* check pointing: ParGenes can be restarted without effort after an interruption
* error reporting for the MSAs that fail to be treated
* options to customize the tree inference parameters
* optional call to ASTRAL/ASTER to infer a species tree from the inferred phylogenetic trees

## Requirement

* Linux or MacOS
* gcc 5.0 or >
* CMake 3.6 or >
* Either MPI or OpenMP. MPI for multiple nodes parallelization (clusters)

## Installation

(Please note that, on Linux, you can also install through [`bioconda`](https://anaconda.org/bioconda/pargenes))

Please use `git`, and clone with `--recursive`!!!

```
git clone --recursive https://github.com/BenoitMorel/ParGenes.git
```

To build the sources:

```
./install.sh
```

To parallelize the compilation with 10 cores:

```
./install.sh 10
```

## Updating the repository

Instead of using: `git pull`, please use: `./gitpull.sh`.

Rational: we use git submodules, and `git pull` might not be enough to update all the changes.
The `gitpull.sh` will update all the changes properly.

## Running

See the wiki (<https://github.com/BenoitMorel/ParGenes/wiki/Running-ParGenes>)

## Documentation and Support

Documentation: in the [github wiki](https://github.com/BenoitMorel/ParGenes/wiki).

Also please check the online help with `python3 pargenes/pargenes.py --help`.

A suggestion, a bug to report, a question? Please use the [RAxML google group](https://groups.google.com/forum/#!forum/raxml).

## Citing

Before citing ParGenes, please make sure you read <https://github.com/BenoitMorel/ParGenes/wiki/Citation>.

