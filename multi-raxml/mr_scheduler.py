import sys
import subprocess
import os
import mr_commons

def get_mpi_scheduler_exec():
  repo_root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  return os.path.join(repo_root, "mpi-scheduler", "build", "mpi-scheduler")

def run_mpi_scheduler(library, commands_filename, output_dir, ranks):
  sys.stdout.flush()
  command = []
  command.append("mpirun")
  command.append("-np")
  command.append(str(ranks))
  command.append(get_mpi_scheduler_exec())
  command.append("--split-scheduler")
  command.append(library)
  command.append(commands_filename)
  command.append(output_dir)
  
  logs_file = os.path.join(output_dir, "logs.txt")
  index = 1
  while (os.path.exists(logs_file)): # maybe from a checkpoint
    logs_file = os.path.join(output_dir, "logs_" + str(index) + ".txt")
    index += 1
  out = open(logs_file, "w")
  print("Calling mpi-scheduler: " + " ".join(command))
  print("Logs will be redirected to " + logs_file)
  p = subprocess.Popen(command, stdout=out, stderr=out)
  p.wait()

