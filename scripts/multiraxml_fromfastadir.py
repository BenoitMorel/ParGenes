import sys
import os
import subprocess
import time

def check_ranks(ranks):
  command = []
  command.append("mpirun")
  #command.append("-display-allocation")
  command.append("-np")
  command.append(str(ranks))
  command.append("sleep")
  command.append("0.1")
  try:
    subprocess.check_call(command)
  except:
    print("Invalid number of ranks: " + str(ranks))
    sys.exit(1)

def getMultiraxmlExec():
  repo_root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  return os.path.join(repo_root, "build", "multi-raxml")

def print_help():
  print("python raxml_runner.py implementation raxml_exec_dir fasta_dir output_dir additionnal_options_file")
  print("implementation: --spawn-scheduler")
  print("             or --mpirun-scheduler")
  print("             or --split-scheduler")

def run_multiraxml(implementation, command_filename, output_dir, ranks):
  sys.stdout.flush()
  command = []
  if (implementation not in ["--mpirun-scheduler", "--spawn-scheduler", "--split-scheduler"]):
    print("ERROR: wrong implementation " + implementation)
    sys.exit(0)
  if (implementation == "--split-scheduler"):
    command.append("mpirun")
    command.append("-np")
    command.append(str(ranks))
  command.append(getMultiraxmlExec())
  command.append(implementation)
  command.append(command_filename)
  command.append(output_dir)
  command.append(str(ranks))
  subprocess.check_call(command)

def build_first_command(raxml_exec_dir, fasta_files, output_dir, options, ranks):
  first_command_file = os.path.join(output_dir, "first_command.txt")
  raxml = os.path.join(raxml_exec_dir, "raxml-ng-mpi")
  first_run_output_dir = os.path.join(output_dir, "first_run")
  first_run_results = os.path.join(first_run_output_dir, "results")
  os.makedirs(first_run_results)
  fasta_chuncks = []
  fasta_chuncks.append([])
  with open(first_command_file, "w") as writer:
    for fasta in fasta_files:
      base = os.path.splitext(os.path.basename(fasta))[0]
      writer.write("first_" + base + " mpi 1 1 ")
      writer.write(raxml)
      writer.write(" --parse ")
      writer.write( " --msa " + fasta + " " + options[:-1])
      writer.write(" --prefix " + os.path.join(first_run_results, base))
      writer.write(" --threads 1 ")
      writer.write("\n")
  return first_command_file

def sites_to_maxcores(sites):
  if sites == 0:
    return 0
  return 1 << ((sites // 1000)).bit_length()

def parse_msa_info(log_file):
  result = [0, 0]
  unique_sites = 0
  try:
    lines = open(log_file).readlines()
  except:
    print("Cannot find file " + log_file)
    return result
  for line in lines:
    if "Alignment comprises" in line:
      unique_sites = int(line.split(" ")[5])
    if "taxa" in line:
      result[1] = int(line.split(" ")[4])
  result[0] = sites_to_maxcores(unique_sites)
  return result

def build_second_command(raxml_exec_dir, fasta_files, output_dir, options, ranks):
  second_command_file = os.path.join(output_dir, "second_command.txt")
  first_run_output_dir = os.path.join(output_dir, "first_run")
  first_run_results = os.path.join(first_run_output_dir, "results")
  second_run_output_dir = os.path.join(output_dir, "second_run")
  second_run_results = os.path.join(second_run_output_dir, "results")
  os.makedirs(second_run_results)
  with open(second_command_file, "w") as writer:
    for fasta in fasta_files:
      base = os.path.splitext(os.path.basename(fasta))[0]
      raxml = os.path.join(raxml_exec_dir, "raxml-ng-mpi")
      first_run_log = os.path.join(os.path.join(first_run_results, base + ".raxml.log"))
      compressed_fasta = os.path.join(os.path.join(first_run_results, base + ".raxml.rba"))
      parse_result = parse_msa_info(first_run_log)
      cores = str(parse_result[0])
      taxa = str(parse_result[1])
      if (cores == "0" or taxa == "0"):
        print("warning with fasta " + fasta + ", skipping")
        continue
      writer.write("second_" + base + " mpi ")
      writer.write(cores + " " + taxa + " " + raxml)
      writer.write(" --msa " + compressed_fasta + " " + options[:-1])
      writer.write(" --prefix " + os.path.join(second_run_results, base))
      writer.write(" --threads 1 ")
      writer.write("\n")
  return second_command_file

def main_raxml_runner(implementation, raxml_exec_dir, fasta_dir, output_dir, options_file, ranks):
  check_ranks(ranks)
  try:
    os.makedirs(output_dir)
  except:
    pass
  print("Results in " + output_dir)
  fasta_files = [os.path.join(fasta_dir, f) for f in os.listdir(fasta_dir)]
  options = open(options_file, "r").readlines()[0]
  first_command_file = build_first_command(raxml_exec_dir, fasta_files, output_dir, options, ranks)
  run_multiraxml(implementation, first_command_file, os.path.join(output_dir, "first_run"), ranks)
  print("### end of first multiraxml run")
  second_command_file = build_second_command(raxml_exec_dir, fasta_files, output_dir, options, ranks)
  print("### end of build_second_command")
  run_multiraxml(implementation, second_command_file, os.path.join(output_dir, "second_run"), ranks)
  print("### end of second multiraxml run")


if (len(sys.argv) != 7):
    print_help()
    sys.exit(0)

implementation = sys.argv[1]
raxml_exec_dir = sys.argv[2]
fasta_dir = sys.argv[3] 
output_dir = sys.argv[4]
options_file = sys.argv[5]
ranks = sys.argv[6]

start = time.time()
main_raxml_runner(implementation, raxml_exec_dir, fasta_dir, output_dir, options_file, ranks)
end = time.time()
print("TOTAL ELAPSED TIME " + str(end-start) + "s")

