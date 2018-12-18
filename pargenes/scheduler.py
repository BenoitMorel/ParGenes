import sys
import subprocess
import os
import commons
import logger
import errorcodes

def print_help_in_error(output_dir):
      logger.error("Please check the logs in " + os.path.join(output_dir, "logs.txt"))
      logger.error("You might need to check individual runs in " + output_dir)
      logger.error("In particular, logs in the subfolders \"per_job_logs\" and results in the folder \"results\"")

def get_mpi_scheduler_exec():
  """ Get the path to the mpi scheduler executable """
  repo_root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  return os.path.join(repo_root, "MPIScheduler", "build", "mpi-scheduler")

def run(command, output_dir, my_env):
  logs_file = commons.get_log_file(output_dir, "logs")
  out = open(logs_file, "w")
  logger.info("Calling mpi-scheduler: " + " ".join(command))
  logger.info("Logs will be redirected to " + logs_file)
  p = subprocess.Popen(command, stdout=out, stderr=out, env=my_env)
  p.wait()
  return p.returncode

def run_mpi_scheduler(library, scheduler, commands_filename, output_dir, ranks, op):
  """ Run the mpi scheduler program """
  sys.stdout.flush()
  command = []
  my_env = os.environ.copy()
  if (scheduler == "openmp"):
    my_env["OMP_NUM_THREADS"] = str(ranks) + "," + str(ranks) + "," + str(ranks)
    my_env["OMP_DYNAMIC"] = "false"
    #command.append("OMP_NUM_TRHEADS=" + str(ranks))
  if (scheduler != "openmp"):
    command.append("mpiexec")
    command.append("-n")
    command.append(str(ranks))
    if (op.valgrind):
      command.append("--mca")
      command.append("btl")
      command.append("tcp,self")
  if (op.valgrind):
    command.append("valgrind")
  command.append(get_mpi_scheduler_exec())
  if (scheduler == "onecore"):
    command.append("--onecore-scheduler")
  elif (scheduler == "split"):
    command.append("--split-scheduler")
  else:
    command.append("--openmp-scheduler")
  command.append(library)
  command.append(commands_filename)
  command.append(output_dir)
  if (op.job_failure_fatal):
    command.append("1")
  else:
    command.append("0")
  errorcode = run(command, output_dir, my_env)
  if (errorcode == 242):
    if (op.job_failure_fatal):
      logger.error("At least one job failed")
      logger.error("Job failures are fatal. To continue when a job fails, do not set --job-failure-fatal")
      print_help_in_error(output_dir)
      logger.error("Aborting")
      report.report_and_exit(op.output_dir, 242)
  if (errorcode != 0): 
    curr_retry = 0
    while (curr_retry < op.retry and errorcode != 0):
      logger.error("mpi-scheduler execution failed with error code " + str(errorcode))
      curr_retry += 1
      logger.error("Retry " + str(curr_retry) + "/" + str(op.retry) + "...")
      errorcode = run(command, output_dir, my_env)

  if (errorcode != 0):
    logger.error("mpi-scheduler execution failed with error code " + str(errorcode))
    logger.error("Will now exit...")
    raise RuntimeError("mpi-scheduler  execution failed with error code " + str(errorcode))
  failed_commands = os.path.join(output_dir, "failed_commands.txt")
  if (os.path.isfile(failed_commands)):
    accumulated_failed_commands = os.path.join(op.output_dir, "failed_commands.txt")
    lines = open(failed_commands).readlines()
    writer = open(accumulated_failed_commands, "a")
    for line in lines:
        writer.write(line)
    
    commands_number = len(open(commands_filename).readlines())
    failed_commands_number = len(lines)
    logger.warning(str(failed_commands_number) + "/" + str(commands_number) + " commands failed")
    if (failed_commands_number == commands_number):
      logger.error("All scheduled commands failed...")
      print_help_in_error(output_dir)
      logger.error("Aborting ParGenes...")
      report.report_and_exit(op.output_dir, errorcodes.ERROR_ALL_COMMAND_FAILED)
