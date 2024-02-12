# ParGenes

A massively parallel tool for model selection and tree inference on thousands of genes

## Features

ParGenes is a parallel tool that takes as input a set of multiple sequence alignments (typically from different genes) and infers their corresponding phylogenetic trees.

ParGenes supports the following features:

* best-fit model selection with [ModelTest-NG](https://github.com/ddarriba/modeltest)
* phylogenetic tree inference with [RAxML-NG](https://github.com/amkozlov/raxml-ng) (with customizable number of starting trees and bootstrap trees)
* automatic parallelization to make the best use of the available computing cores
* check pointing: ParGenes can be restarted without effort after an interruption
* error reporting for the MSAs that fail to be treated
* options to customize the tree inference parameters
* optional call to [ASTRAL](https://github.com/smirarab/ASTRAL)/[ASTER](https://github.com/chaoszhang/ASTER) to infer a species tree from the inferred phylogenetic trees

## Requirements

* Linux or MacOS
* gcc 5.0 or >
* CMake 3.6 or >
* Either MPI or OpenMP. MPI for multiple nodes parallelization (clusters)

## Installation

(Please note that, on Linux, you can also install through [`bioconda`](https://anaconda.org/bioconda/pargenes))

Please use `git`, and clone with `--recursive`!

    $ git clone --recursive https://github.com/BenoitMorel/ParGenes.git

To build the sources:

    $ ./install.sh

To parallelize the compilation with 10 cores:

    $ ./install.sh 10

**To install with ASTER capacity, use the following:**

    $ git clone --recurse-submodules https://github.com/nylander/ParGenes.git
    $ cd ParGenes
    $ git checkout aster
    $ git submodule update --init --recursive
    $ ./install.sh

## Updating the repository

Instead of using: `git pull`, please use: `./gitpull.sh`.

Rational: we use git submodules, and `git pull` might not be enough to update all the changes.
The `gitpull.sh` will update all the changes properly.

## Running

See the wiki (<https://github.com/BenoitMorel/ParGenes/wiki/Running-ParGenes>).

#### Run with ASTER

As an alternative to the default ASTRAL III software, the newer species-tree
implementations from ASTER can be used. Specifically, the (ASTER) programs
`astral`, `astral-hybrid` and `astral-pro` are made available.  The added
options for pargenes are:

- `--use-aster`  Use ASTER instead of ASTRAL. Note: should not be combined with `--use-astral`.
- `--aster-bin <name or path>`  Name or path to programs `astral`, `astral-hybrid`, or `astral-pro`.
- `--aster-global-parameters <text file>` Pass extra parameters to any of the chosen ASTER programs in a text file.

See <https://github.com/chaoszhang/ASTER> for more description of the ASTER implementations.

## Documentation and Support

Documentation: in the [github wiki](https://github.com/BenoitMorel/ParGenes/wiki).

Also please check the online help with `python3 pargenes/pargenes.py --help`.

A suggestion, a bug to report, a question? Please use the [RAxML google group](https://groups.google.com/forum/#!forum/raxml).

## Citing

Before citing ParGenes, please make sure you read <https://github.com/BenoitMorel/ParGenes/wiki/Citation>.

