import argparse
import sys
import os
import logger
import version

def exit_msg(msg):
  logger.error(msg)
  sys.exit(1)

def check_mandatory_field(f, name):
  if (f == None or len(f) == 0):
    exit_msg("Error: please provide the " + name)

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
  parser.add_argument('-V', '--version',
    action='version',
    version="ParGenes ("+version.get_pargenes_version_string()+")")
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
    choices=["split", "onecore", "fork"],
    default="split",
    help="Expert-user only.")
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
  parser.add_argument("--constrain-search",
    dest="constrain_search",
    action="store_true",
    default=False,
    help="Constrain the raxml searches")
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
  parser.add_argument("--autoMRE",
    dest="autoMRE",
    action="store_true",
    default=False,
    help="Stop computing bootstrap trees after autoMRE bootstrap convergence test. You have to specify the maximum number of bootstrap trees with -b or --bs-tree")
  parser.add_argument("--raxml-binary",
    dest="raxml_binary",
    default="",
    help="Custom path to raxml-ng executable. Please refer to the wiki before setting this variable yourself.")
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
  parser.add_argument("--modeltest-binary",
    dest="modeltest_binary",
    default="",
    help="Custom path to modeltest-ng executable. Please refer to the wiki before setting this variable yourself.")
  # astral arguments
  parser.add_argument("--use-astral",
    dest="use_astral",
    action="store_true",
    default=False,
    help="Infer a species tree with astral")
  parser.add_argument("--astral-global-parameters",
    dest="astral_global_parameters",
    help="A file containing additional parameters to pass to astral")
  parser.add_argument("--astral-jar",
    dest="astral_jar",
    default="",
    help="Custom path to ASTRAL jar file.")
  # aster arguments
  parser.add_argument("--use-aster",
    dest="use_aster",
    action="store_true",
    default=False,
    help="Infer a species tree with aster. Default is to use (aster) astral, but this can be changed with option --aster-bin")
  parser.add_argument("--aster-global-parameters",
    dest="aster_global_parameters",
    help="A file containing additional parameters to pass to aster")
  parser.add_argument("--aster-bin",
    dest="aster_bin",
    default="",
    help="Name or custom path to aster binary file (astral|astral-hybrid|astral-pro)")
  # experiments
  parser.add_argument("--experiment-disable-jobs-sorting",
    dest="disable_job_sorting",
    action="store_true",
    default=False,
    help="For experimenting only! Removes the sorting step in the scheduler")
  parser.add_argument("--retry",
    dest="retry",
    type=int,
    default=0,
    help="Number of time the scheduler should try to restart after an error")
  op = parser.parse_args()
  print("after parse:")
  check_argument_dir(op.alignments_dir, "alignment")
  check_argument_file(op.msa_filter, "msa filter")
  check_argument_file(op.per_msa_raxml_parameters, "per_msa_raxml_parameters")
  check_argument_file(op.raxml_global_parameters, "raxml_global_parameters")
  check_argument_file(op.per_msa_modeltest_parameters, "per_msa_modeltest_parameters")
  check_argument_file(op.modeltest_global_parameters, "modeltest_global_parameters")
  check_argument_file(op.astral_global_parameters, "astral_global_parameters")
  check_argument_file(op.aster_global_parameters, "aster_global_parameters")
  check_mandatory_field(op.alignments_dir, "alignment directory (\"-a\"")
  check_mandatory_field(op.output_dir, "output directory (\"-o\")")
  if (op.autoMRE and op.bootstraps < 1):
    exit_msg("When using autoMRE option, you need to specify the maximum number of boostraps with --bs-trees")
  if (not len(os.listdir(op.alignments_dir))):
    exit_msg("Please provide a non empty alignments directory.")
  if (op.cores < 2):
    exit_msg("Please set the number of cores (--cores or -c) to at least 2")
  return op

