# ParGenes 

A massively parallel tool for model selection and tree inference on thousands of genes

ParGenes is for you if:
* you have several MSAs (typically gene alignments).
* you want to run maximum likelihood tree inference (RAxML) independently on each of them. For instance, to get one gene tree per gene alignment.
* you want to run these jobs in parallel, (single or multiple nodes).

In addition, ParGenes:
* can find the best-fit model with ModelTest and use this model in the RAxML calls.
* has a checkpoint mechanism.
* filters out and reports a list of the MSAs that RAxML can not process.
* handles multiple starting trees, bootstrap replicates, support value. ParGenes can run these searches simultaneously, and thus improves RAxML parallelization scheme.  
* provides a (global or per-MSA) way to customize the modeltest and RAxML calls.
* can infer the optimal number of cores to assign to a given ParGenes call.

## Requirement

* Git
* CMake 3.6 or >
* gcc 5.0 or >
* Either MPI or OpenMP
* Python 3

## Installation

Please use git,  and clone with --recursive!!!

```
git clone --recursive git@github.com:BenoitMorel/ParGenes.git
```

To build the sources:
```
./install.sh
```
## Running

See the wiki (https://github.com/BenoitMorel/ParGenes/wiki)

## Documentation and Support

Documentation: in the [github wiki](https://github.com/BenoitMorel/ParGenes/wiki) (work in progress).

Also please check the online help with `python3 pargenes/pargenes.py --help`

A suggestion, a bug to report, a question? Please use the [RAxML google group](https://groups.google.com/forum/#!forum/raxml).


