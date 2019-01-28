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
import report
import datetime


def print_header(args):
  logger.info("########################")
  logger.info("#    PARGENES v1.0.1   #")
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
  raxml_run_path = os.path.join(output_dir, "mlsearch_run")
  if (op.scheduler != "split"):
    raxml_library = os.path.join(scriptdir, "..", "raxml-ng", "bin", "raxml-ng")
    modeltest_library = os.path.join(scriptdir, "..", "modeltest", "bin", "modeltest-ng")
  else:
    raxml_library = os.path.join(scriptdir, "..", "raxml-ng", "bin", "raxml-ng-mpi.so")
    modeltest_library = os.path.join(scriptdir, "..", "modeltest", "build", "src", "modeltest-ng-mpi.so")
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
    if (checkpoint_index < 6):
      bootstraps.run(output_dir, raxml_library, op.scheduler, os.path.join(output_dir, "supports_run"), op.cores, op)
      logger.timed_log("end of supports mpi-scheduler run")
      checkpoint.write_checkpoint(output_dir, 6)
  if (op.use_astral):
    if (checkpoint_index < 7):
      astral_jar = os.path.join(scriptdir, "..", "ASTRAL", "Astral", "astral.jar")
      astral.run_astral_pargenes(astral_jar,  op)
      checkpoint.write_checkpoint(output_dir, 7)
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


