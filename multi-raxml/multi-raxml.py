import sys
import os
import time
from datetime import timedelta
import glob
import mr_arguments
import mr_commons
import mr_raxml
import mr_bootstraps
import mr_modeltest
import mr_scheduler
import mr_checkpoint

def print_header():
  print("#################")
  print("#  MULTI RAXML  #")
  print("#################")
  print("Multi-raxml was called as follow:")
  print(" ".join(sys.argv))
  print("")


def timed_print(initial_time, msg):
  """ Print the time elapsed from initial_time and the msg """ 
  elapsed = time.time() - initial_time
  print("### [" + str(timedelta(seconds = int(elapsed))) + "] " + msg)

def main_raxml_runner(op): 
  """ Run multi-raxml from the parsed arguments op """
  start = time.time()
  output_dir = op.output_dir
  checkpoint = mr_checkpoint.read_checkpoint(output_dir)
  print("Checkpoint: " + str(checkpoint))
  if (os.path.exists(output_dir) and not op.do_continue):
    print("[Error] The output directory " + output_dir + " already exists. Please use another output directory.")
    sys.exit(1)
  mr_commons.makedirs(output_dir)
  logs = mr_commons.get_log_file(output_dir, "multi_raxml_logs")
  print("Redirecting logs to " + logs)
  sys.stdout = open(logs, "w")
  print_header()
  msas = mr_commons.init_msas(op)
  timed_print(start, "end of MSAs initializations")
  scriptdir = os.path.dirname(os.path.realpath(__file__))
  modeltest_run_path = os.path.join(output_dir, "modeltest_run")
  raxml_run_path = os.path.join(output_dir, "mlsearch_run")
  if (op.scheduler == "onecore"):
    raxml_library = os.path.join(scriptdir, "..", "raxml-ng", "bin", "raxml-ng")
    modeltest_library = os.path.join(scriptdir, "..", "modeltest", "bin", "modeltest-ng")
  else:
    raxml_library = os.path.join(scriptdir, "..", "raxml-ng", "bin", "raxml-ng-mpi.so")
    modeltest_library = os.path.join(scriptdir, "..", "modeltest", "build", "src", "modeltest-ng-mpi.so")
  if (checkpoint < 1):
    mr_raxml.run_parsing_step(msas, raxml_library, op.scheduler, os.path.join(output_dir, "parse_run"), op.cores, op)
    mr_checkpoint.write_checkpoint(output_dir, 1)
    timed_print(start, "end of parsing mpi-scheduler run")
  mr_raxml.analyse_parsed_msas(msas, op, output_dir)
  timed_print(start, "end of anlysing parsing results") 
  if (op.use_modeltest):
    if (checkpoint < 2):
      mr_modeltest.run(msas, output_dir, modeltest_library, modeltest_run_path, op)
      timed_print(start, "end of modeltest mpi-scheduler run")
      mr_checkpoint.write_checkpoint(output_dir, 2)
    mr_modeltest.parse_modeltest_results(op.modeltest_criteria, msas, output_dir)
    timed_print(start, "end of parsing  modeltest results")
  if (checkpoint < 3):
    mr_raxml.run(msas, op.random_starting_trees, op.parsimony_starting_trees, op.bootstraps, raxml_library, op.scheduler, raxml_run_path, op.cores, op)
    timed_print(start, "end of mlsearch mpi-scheduler run")
    mr_checkpoint.write_checkpoint(output_dir, 3)
  if (op.random_starting_trees + op.parsimony_starting_trees > 1):
    if (checkpoint < 4):
      mr_raxml.select_best_ml_tree(msas, op)
      timed_print(start, "end of selecting the best ML tree")
      mr_checkpoint.write_checkpoint(output_dir, 4)
  if (op.bootstraps != 0):
    if (checkpoint < 5):
      mr_bootstraps.concatenate_bootstraps(output_dir, op.cores)
      timed_print(start, "end of bootstraps concatenation")
      mr_checkpoint.write_checkpoint(output_dir, 5)
    if (checkpoint < 6):
      mr_bootstraps.run(output_dir, raxml_library, op.scheduler, os.path.join(output_dir, "supports_run"), op.cores, op)
      timed_print(start, "end of supports mpi-scheduler run")
      mr_checkpoint.write_checkpoint(output_dir, 6)

print_header()
start = time.time()
main_raxml_runner(mr_arguments.parse_arguments())
end = time.time()
timed_print(start, "END OF THE RUN OF " + os.path.basename(__file__))

