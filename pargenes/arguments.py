import argparse
import sys
import os
import logger

def exit_msg(msg):
  logger.error(msg)
  sys.exit(1)

def check_argument_file(f, name):
  if (None != f and not os.path.isfile(f)):
    exit_msg("Error: invalid " + name + " file: " + f)

def check_argument_dir(f, name):
  if (None != f and not os.path.isdir(f)):
    exit_msg("Error: invalid " + name + " directory: " + f)

# parse the command line and return the arguments
def parse_arguments(args):
  parser = argparse.ArgumentParser(args)
  # general arguments
  
  parser.add_argument("--dry-run",
      dest="dry_run",
      action="store_true",
      default=False,
      help="")
  parser.add_argument('-a', "--alignments-dir", 
      dest="alignments_dir", 
      help="Directory containing the fasta files")
  parser.add_argument('-o', "--output-dir", 
      dest="output_dir", 
      help="Output directory")
  parser.add_argument("-c", "--cores", 
      dest="cores",
      type=int,
      help="The number of computational cores available for computation. Should at least 2.")
  parser.add_argument("--seed", 
      dest="seed",
      type=int,
      help="Random seed, for reproductibility of RAxML-NG runs. Set to 0 by default",
      default=0)
  parser.add_argument("--continue",
      dest="do_continue",
      action="store_true",
      default=False,
      help="Allow pargenes to continue the analysis from the last checkpoint")
  parser.add_argument("--msa-filter",
      dest="msa_filter", 
      help="A file containing the names of the msa files to process")
  parser.add_argument("-d", "--datatype",
      dest="datatype",
      choices=["nt", "aa"],
      default="nt",
      help="Alignments datatype")
  parser.add_argument("--scheduler",
      dest="scheduler",
      choices=["split", "onecore", "openmp"],
      default="split",
      help="Sceduling strategy. Prefer split for multiple nodes platforms, and openmp else (for instance when running on your personal computer.")
  parser.add_argument("--core-assignment",
      dest="core_assignment",
      choices=["high", "medium", "low"],
      default="medium",
      help="Policy to decide the per-job number of cores (low favors a low per-job number of cores)")
  parser.add_argument("--valgrind",
      dest="valgrind",
      action="store_true",
      default=False,
      help="Run the scheduler with valgrind")
  parser.add_argument("--job-failure-fatal",
      dest="job_failure_fatal",
      action="store_true",
      default=False,
      help="Stop ParGenes when a job fails")
  # raxml arguments
  parser.add_argument("--per-msa-raxml-parameters", 
      dest="per_msa_raxml_parameters", 
      help="A file containing per-msa raxml parameters")
  parser.add_argument("-s", "--random-starting-trees", 
      dest="random_starting_trees", 
      type=int,
      default=1,
      help="The number of starting trees")
  parser.add_argument("-p", "--parsimony-starting-trees", 
      dest="parsimony_starting_trees", 
      type=int,
      default=0,
      help="The number of starting parsimony trees")
  parser.add_argument("-r", "--raxml-global-parameters", 
      dest="raxml_global_parameters", 
      help="A file containing the parameters to pass to raxml")
  parser.add_argument("-R", "--raxml-global-parameters-string", 
      dest="raxml_global_parameters_string", 
      help="List of parameters to pass to raxml (see also --raxml-global-parameters)")
  parser.add_argument("-b", "--bs-trees", 
      dest="bootstraps", 
      type=int,
      default=0,
      help="The number of bootstrap trees to compute")
  parser.add_argument("--percentage-jobs-double-cores",
      dest="percentage_jobs_double_cores",
      type=float,
      default=0.05,
      help="Percentage (between 0 and 1) of jobs that will receive twice more cores")
  # modeltest arguments
  parser.add_argument("-m", "--use-modeltest",
      dest="use_modeltest",
      action="store_true",
      default=False,
      help="Autodetect the model with modeltest")
  parser.add_argument("--modeltest-global-parameters", 
      dest="modeltest_global_parameters", 
      help="A file containing the parameters to pass to modeltest")
  parser.add_argument("--per-msa-modeltest-parameters",
      dest="per_msa_modeltest_parameters", 
      help="A file containing per-msa modeltest parameters")
  parser.add_argument("--modeltest-criteria",
      dest="modeltest_criteria",
      choices=["AICc", "AIC", "BIC"],
      default="AICc",
      help="Alignments datatype")
  parser.add_argument("--modeltest-perjob-cores",
      dest="modeltest_cores",
      type=int,
      default=16,
      help="Number of cores to assign to each modeltest core (at least 4)")
  # astral arguments
  parser.add_argument("--use-astral",
      dest="use_astral",
      action="store_true",
      default=False,
      help="Infer a species tree with astral")
  parser.add_argument("--astral-global-parameters", 
      dest="astral_global_parameters", 
      help="A file containing additional parameters to pass to astral")
  # experiments
  parser.add_argument("--experiment-disable-jobs-sorting",
      dest="disable_job_sorting",
      action="store_true",
      default=False,
      help="For experimenting only! Removes the sorting step in the scheduler")
  

  op = parser.parse_args()
  check_argument_dir(op.alignments_dir, "alignment")
  check_argument_file(op.msa_filter, "msa filter")
  check_argument_file(op.per_msa_raxml_parameters, "per_msa_raxml_parameters")
  check_argument_file(op.raxml_global_parameters, "raxml_global_parameters")
  check_argument_file(op.per_msa_modeltest_parameters, "per_msa_modeltest_parameters")
  check_argument_file(op.modeltest_global_parameters, "modeltest_global_parameters")
  if (op.cores < 2):
    exit_msg("Please set the number of cores (--cores or -c) to at least 2")
  return op

