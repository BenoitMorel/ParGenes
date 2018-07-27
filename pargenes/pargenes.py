import sys
import os
import time
from datetime import timedelta
import glob
import arguments
import commons
import raxml
import bootstraps
import modeltest
import scheduler
import checkpoint
import shutil

def print_header():
  print("##################")
  print("#    PARGENES    #")
  print("##################")
  print("ParGenes was called as follow:")
  print(" ".join(sys.argv))
  print("")


def timed_print(initial_time, msg):
  """ Print the time elapsed from initial_time and the msg """ 
  elapsed = time.time() - initial_time
  print("### [" + str(timedelta(seconds = int(elapsed))) + "] " + msg)



def main_raxml_runner(op): 
  """ Run pargenes from the parsed arguments op """
  start = time.time()
  output_dir = op.output_dir
  checkpoint_index = checkpoint.read_checkpoint(output_dir)
  print("Checkpoint: " + str(checkpoint_index))
  if (os.path.exists(output_dir) and not op.do_continue):
    print("[Error] The output directory " + output_dir + " already exists. Please use another output directory or run with --continue.")
    sys.exit(1)
  commons.makedirs(output_dir)
  logs = commons.get_log_file(output_dir, "pargenes_logs")
  print("Redirecting logs to " + logs)
  sys.stdout = open(logs, "w")
  print_header()
  msas = commons.init_msas(op)
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
  if (checkpoint_index < 1):
    raxml.run_parsing_step(msas, raxml_library, op.scheduler, os.path.join(output_dir, "parse_run"), op.cores, op)
    checkpoint.write_checkpoint(output_dir, 1)
    timed_print(start, "end of parsing mpi-scheduler run")
  raxml.analyse_parsed_msas(msas, op, output_dir)
  if (op.dry_run):
    print("End of the dry run. Exiting")
    return 0
  timed_print(start, "end of anlysing parsing results") 
  if (op.use_modeltest):
    if (checkpoint_index < 2):
      modeltest.run(msas, output_dir, modeltest_library, modeltest_run_path, op)
      timed_print(start, "end of modeltest mpi-scheduler run")
      modeltest.parse_modeltest_results(op.modeltest_criteria, msas, output_dir)
      timed_print(start, "end of parsing  modeltest results")
      # then recompute the binary MSA files to put the correct model, and reevaluate the MSA sizes with the new models
      shutil.move(os.path.join(output_dir, "parse_run"), os.path.join(output_dir, "old_parse_run"))
      raxml.run_parsing_step(msas, raxml_library, op.scheduler, os.path.join(output_dir, "parse_run"), op.cores, op)
      raxml.analyse_parsed_msas(msas, op, output_dir)
      timed_print(start, "end of the second parsing step") 
      checkpoint.write_checkpoint(output_dir, 2)
  if (checkpoint_index < 3):
    raxml.run(msas, op.random_starting_trees, op.parsimony_starting_trees, op.bootstraps, raxml_library, op.scheduler, raxml_run_path, op.cores, op)
    timed_print(start, "end of mlsearch mpi-scheduler run")
    checkpoint.write_checkpoint(output_dir, 3)
  if (op.random_starting_trees + op.parsimony_starting_trees > 1):
    if (checkpoint_index < 4):
      raxml.select_best_ml_tree(msas, op)
      timed_print(start, "end of selecting the best ML tree")
      checkpoint.write_checkpoint(output_dir, 4)
  if (op.bootstraps != 0):
    if (checkpoint_index < 5):
      bootstraps.concatenate_bootstraps(output_dir, min(16, op.cores))
      timed_print(start, "end of bootstraps concatenation")
      checkpoint.write_checkpoint(output_dir, 5)
    if (checkpoint_index < 6):
      bootstraps.run(output_dir, raxml_library, op.scheduler, os.path.join(output_dir, "supports_run"), op.cores, op)
      timed_print(start, "end of supports mpi-scheduler run")
      checkpoint.write_checkpoint(output_dir, 6)
  return 0

save_cout = sys.stdout
print_header()
start = time.time()
ret =  main_raxml_runner(arguments.parse_arguments())
end = time.time()
timed_print(start, "END OF THE RUN OF " + os.path.basename(__file__))
sys.stdout = save_cout
if (ret != 0):
  print("Something went wrong, please check the logs") 

