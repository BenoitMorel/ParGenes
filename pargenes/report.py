import sys
import os

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
      print("[Report] Checkpoint " + str(iteration))
      writer.write(f.read() + "\n")
  except:
    pass


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

if (len(sys.argv) != 3):
  print("Syntax: python report.py pargenes_directory report_file")
  print("  pargenes directory is the path to your pargenes run (corresponds to -o when you called pargenes)")
  print("  report_file is the path to the report file that will be generated (for instance report.txt)")
  print("When reporting an issue, please send us the report_file")
  sys.exit(1)

pargenes_dir = sys.argv[1]
output = sys.argv[2]

report(pargenes_dir, output)
