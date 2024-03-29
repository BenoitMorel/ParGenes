import sys
import os
import time
import glob
import arguments
import commons
import raxml
import bootstraps
import modeltest
import scheduler
import checkpoint
import shutil
import logger
import astral
import aster
import report
import datetime
import version
import constraint

def print_header(args):
  logger.info("########################")
  logger.info("#    PARGENES " + version.get_pargenes_version_string() + "   #")
  logger.info("########################")
  logger.info("")
  print(datetime.datetime.now())
  logger.info("ParGenes was called as follow:")
  logger.info(" ".join(args))
  logger.info("")

def print_stats(op):
  failed_commands = os.path.join(op.output_dir, "failed_commands.txt")
  if (os.path.isfile(failed_commands)):
    failed_number = len(open(failed_commands).readlines())
    logger.warning("Total number of jobs that failed: " + str(failed_number))
    logger.warning("For a detailed list, see " + failed_commands)

def main_raxml_runner(args, op):
  """ Run pargenes from the parsed arguments op """
  start = time.time()
  output_dir = op.output_dir
  checkpoint_index = checkpoint.read_checkpoint(output_dir)
  if (os.path.exists(output_dir) and not op.do_continue):
    logger.info("[Error] The output directory " + output_dir + " already exists. Please use another output directory or run with --continue.")
    sys.exit(1)
  commons.makedirs(output_dir)
  logger.init_logger(op.output_dir)
  print_header(args)
  msas = None
  logger.timed_log("end of MSAs initializations")
  scriptdir = os.path.dirname(os.path.realpath(__file__))
  modeltest_run_path = os.path.join(output_dir, "modeltest_run")
  constrain_run_path = os.path.join(output_dir, "constrain_run")
  raxml_run_path = os.path.join(output_dir, "mlsearch_run")
  binaries_dir = os.path.join(scriptdir, "..", "pargenes_binaries")
  print("Binaries directory: " + binaries_dir)
  if (op.scheduler != "split"):
    raxml_library = os.path.join(binaries_dir, "raxml-ng")
    modeltest_library = os.path.join(binaries_dir, "modeltest-ng")
  else:
    raxml_library = os.path.join(binaries_dir, "raxml-ng-mpi.so")
    modeltest_library = os.path.join(binaries_dir, "modeltest-ng-mpi.so")
  if (op.raxml_binary):
    raxml_library = op.raxml_binary
  if (op.modeltest_binary):
    modeltest_library = op.modeltest_binary
  if (op.use_astral):
    if (op.astral_jar):
      astral_jar = os.path.abspath(op.astral_jar)
    else:
      astral_jar = os.path.join(binaries_dir, "astral.jar")
  elif (op.use_aster):
    if (op.aster_bin):
      if '/' in op.aster_bin:
        aster_bin = os.path.abspath(op.aster_bin)
      else:
        aster_bin = os.path.join(binaries_dir, op.aster_bin)
    else:
      aster_bin = os.path.join(binaries_dir, "astral")
  if (checkpoint_index < 1):
    msas = commons.init_msas(op)
    raxml.run_parsing_step(msas, raxml_library, op.scheduler, os.path.join(output_dir, "parse_run"), op.cores, op)
    raxml.analyse_parsed_msas(msas, op)
    checkpoint.write_checkpoint(output_dir, 1)
    logger.timed_log("end of parsing mpi-scheduler run")
  else:
    msas = raxml.load_msas(op)
  if (op.dry_run):
    logger.info("End of the dry run. Exiting")
    return 0
  logger.timed_log("end of anlysing parsing results")
  if (op.use_modeltest):
    if (checkpoint_index < 2):
      modeltest.run(msas, output_dir, modeltest_library, modeltest_run_path, op)
      logger.timed_log("end of modeltest mpi-scheduler run")
      modeltest.parse_modeltest_results(op.modeltest_criteria, msas, output_dir)
      logger.timed_log("end of parsing  modeltest results")
      # then recompute the binary MSA files to put the correct model, and reevaluate the MSA sizes with the new models
      shutil.move(os.path.join(output_dir, "parse_run"), os.path.join(output_dir, "old_parse_run"))
      raxml.run_parsing_step(msas, raxml_library, op.scheduler, os.path.join(output_dir, "parse_run"), op.cores, op)
      raxml.analyse_parsed_msas(msas, op)
      logger.timed_log("end of the second parsing step")
      checkpoint.write_checkpoint(output_dir, 2)
  if (checkpoint_index < 3):
    print("TODO UPDATE CHECKPOINT")
    if (op.constrain_search):
      print("Computing consensus trees ")
      samples = 100
      constraint.compute_constrain(msas, samples, raxml_library, op.scheduler, constrain_run_path, op.cores, op)
      logger.timed_log("end of consensus tree building")
  if (checkpoint_index < 3):
    raxml.run(msas, op.random_starting_trees, op.parsimony_starting_trees, op.bootstraps, raxml_library, op.scheduler, raxml_run_path, op.cores, op)
    logger.timed_log("end of mlsearch mpi-scheduler run")
    checkpoint.write_checkpoint(output_dir, 3)
  if (op.random_starting_trees + op.parsimony_starting_trees > 1):
    if (checkpoint_index < 4):
      raxml.select_best_ml_tree(msas, op)
      logger.timed_log("end of selecting the best ML tree")
      checkpoint.write_checkpoint(output_dir, 4)
  if (op.bootstraps != 0):
    if (checkpoint_index < 5):
      bootstraps.concatenate_bootstraps(output_dir, min(16, op.cores))
      logger.timed_log("end of bootstraps concatenation")
      checkpoint.write_checkpoint(output_dir, 5)
    starting_trees = op.random_starting_trees + op.parsimony_starting_trees
    if (checkpoint_index < 6 and starting_trees > 0):
      bootstraps.run(msas, output_dir, raxml_library, op.scheduler, os.path.join(output_dir, "supports_run"), op.cores, op)
      logger.timed_log("end of supports mpi-scheduler run")
      checkpoint.write_checkpoint(output_dir, 6)
  if (op.use_astral):
    if (checkpoint_index < 7):
      astral.run_astral_pargenes(astral_jar,  op)
      checkpoint.write_checkpoint(output_dir, 7)
  elif (op.use_aster):
    if (checkpoint_index < 7):
      aster.run_aster_pargenes(aster_bin,  op)
      checkpoint.write_checkpoint(output_dir, 7)
  all_invalid = True
  for name, msa in msas.items():
    if (msa.valid):
      all_invalid = False
  if (all_invalid):
    print("[Error] ParGenes failed to analyze all MSAs.")
    report.report_and_exit(op.output_dir, 1)
  print_stats(op)
  return 0

def run_pargenes(args):
  start = time.time()
  ret = 0
  try:
    op = arguments.parse_arguments(args)
  except Exception as inst:
    logger.info("[Error] " + str(type(inst)) + " " + str(inst))
    sys.exit(1)
  try:
    ret =  main_raxml_runner(args, op)
  except Exception as inst:
    logger.info("[Error] " + str(type(inst)) + " " + str(inst))
    report.report_and_exit(op.output_dir, 1)
  end = time.time()
  logger.timed_log("END OF THE RUN OF " + os.path.basename(__file__))
  if (ret != 0):
    logger.info("Something went wrong, please check the logs")

if __name__ == '__main__':
  run_pargenes(sys.argv)

