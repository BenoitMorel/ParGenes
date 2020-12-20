import sys
import os
import logger
import subprocess

def write_header(writer, title):
    title = "** " + "[REPORT] " + title + " **"
    stars = "*" * len(title) + "\n"
    title = title + "\n"
    writer.write("\n")
    writer.write(stars)
    writer.write(title)
    writer.write(stars)
    writer.write("\n")

def extract_file(file_path, title, writer):
  write_header(writer, title)
  try:
    writer.write(open(file_path).read() + "\n")
  except:
    writer.write("Failed to extract the file" + file_path + "\n")
  # if restarting from a checkpoint
  iteration = 1
  try:
    while(True):
      f = open(file_path[:-4] + "_" + str(iteration) + ".txt")
      writer.write("[Report] Checkpoint " + str(iteration) + "\n")
      writer.write(f.read() + "\n")
      iteration = iteration + 1
  except:
    pass

def extract_directory_content(dir_path, title, writer):
  write_header(writer, title)
  try:
    writer.write("Content of directory " + dir_path + ":\n")
    for doc in os.listdir(dir_path):
      writer.write(doc + "\n")
    writer.write("\n")
  except:
    writer.write("Failed to get content of directory " + dir_path + "\n")
    
    
def extract_running_logs(running_dir, logs_dir, writer):
  try:
    for job in os.listdir(running_dir):
      write_header(writer, "running job " + job)
      log_file = os.path.join(logs_dir, job + "_out.txt")
      extract_file(log_file, "running log " + job, writer)
  except:
    writer.write("Could not extract running logs from " + running_dir)

def call_and_log(command, path, writer):
  write_header(writer, " ".join(command) + ":")
  writer.flush()
  if (len(path) > 0):
    subprocess.check_call(command, cwd = path, stdout = writer)
  else:
    subprocess.check_call(command, stdout = writer)
  writer.flush()
  writer.write("\n")

def extract_git(writer):
  try:
    gitpath = os.path.dirname(os.path.realpath(__file__))
    call_and_log(["git", "log", "HEAD^..HEAD"], gitpath, writer)
    call_and_log(["git", "status"], gitpath, writer)
    call_and_log(["git", "diff"], gitpath, writer)
  except:
    writer.write("failed to get ParGenes git commit\n")

def extract_mpi(writer):
  try: 
    call_and_log(["mpiexec", "--version"], "", writer)
  except:
    writer.write("Could not get mpi version")

def extract_arch(writer):
  try: 
    call_and_log(["lscpu"], "", writer)
  except:
    writer.write("Could not get architecture information")

def report(pargenes_dir, output):
  writer = open(output, "w")
  writer.write("ParGenes report file for run " + pargenes_dir)
  
  main_logs = os.path.join(pargenes_dir, "pargenes_logs.txt")
  oldparse_dir = os.path.join(pargenes_dir, "old_parse_run")
  oldparse_logs = os.path.join(oldparse_dir, "logs.txt")
  
  extract_file(main_logs, "MainLogs",  writer)
  steps = ["parse", "old_parse", "modeltest", "mlsearch", "supports"]
  for step in steps:
    step_dir = os.path.join(pargenes_dir, step + "_run")
    logs = os.path.join(step_dir, "logs.txt")
    extract_file(logs, step + " logs", writer)
  
  
  for step in steps:
    step_dir = os.path.join(pargenes_dir, step + "_run")
    command_logs = os.path.join(step_dir, step + "_command.txt")
    failures_logs = os.path.join(step_dir, "failed_commands.txt")
    if (step == "old_parse"):
      command_logs = os.path.join(step_dir, "parse" + "_command.txt")
    extract_file(command_logs, step + " commands", writer)
    extract_file(failures_logs, step + " failed commands", writer)

  for step in steps:
    step_dir = os.path.join(pargenes_dir, step + "_run")
    running_dir = os.path.join(step_dir, "running_jobs")
    extract_directory_content(running_dir, step + " running jobs", writer)
  
  for step in steps:
    step_dir = os.path.join(pargenes_dir, step + "_run")
    running_dir = os.path.join(step_dir, "running_jobs")
    logs_dir = os.path.join(step_dir, "per_job_logs")
    extract_running_logs(running_dir, logs_dir, writer)
  
  extract_git(writer)
  extract_mpi(writer)
  extract_arch(writer)
  writer.close()

def report_and_exit(output_dir, exit_code):
  if (output_dir == None or not len(output_dir)):
    sys.exit(exit_code)
  report_file = os.path.abspath(os.path.join(output_dir, "report.txt"))
  logger.info("Writing report file in " + report_file)
  logger.info("When reporting the issue, please always send us this file.")
  report(output_dir, report_file)
  sys.exit(exit_code)

if __name__ == '__main__':

  if (len(sys.argv) != 3):
    print("Syntax: python report.py pargenes_directory report_file")
    print("  pargenes directory is the path to your pargenes run (corresponds to -o when you called pargenes)")
    print("  report_file is the path to the report file that will be generated (for instance report.txt)")
    print("When reporting an issue, please send us the report_file")
    sys.exit(1)

  pargenes_dir = sys.argv[1]
  output = sys.argv[2]

  report(pargenes_dir, output)
