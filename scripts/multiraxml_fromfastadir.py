import sys
import os
import subprocess
import time

def getMultiraxmlExec():
  repo_root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  return os.path.join(repo_root, "build", "multi-raxml")

def run_multiraxml(raxml_library, command_filename, output_dir, ranks):
  sys.stdout.flush()
  command = []
  command.append("mpirun")
  command.append("-np")
  command.append(str(ranks))
  command.append(getMultiraxmlExec())
  command.append(implementation)
  command.append(raxml_library)
  command.append(command_filename)
  command.append(output_dir)
  print ("Calling multiraxml: " + " ".join(command))
  subprocess.check_call(command)

def build_first_command(raxml_exec_dir, fasta_files, output_dir, options, ranks):
  first_command_file = os.path.join(output_dir, "first_command.txt")
  first_run_output_dir = os.path.join(output_dir, "first_run")
  first_run_results = os.path.join(first_run_output_dir, "results")
  os.makedirs(first_run_results)
  fasta_chuncks = []
  fasta_chuncks.append([])
  with open(first_command_file, "w") as writer:
    for fasta in fasta_files:
      base = os.path.splitext(os.path.basename(fasta))[0]
      fasta_output_dir = os.path.join(first_run_results, base)
      os.makedirs(fasta_output_dir)
      writer.write("first_" + base + " 1 1 ")
      writer.write(" --parse ")
      writer.write( " --msa " + fasta + " " + options[:-1])
      writer.write(" --prefix " + os.path.join(fasta_output_dir, base))
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
      first_fasta_output_dir = os.path.join(first_run_results, base)
      second_fasta_output_dir = os.path.join(second_run_results, base)
      os.makedirs(second_fasta_output_dir)
      first_run_log = os.path.join(os.path.join(first_fasta_output_dir, base + ".raxml.log"))
      compressed_fasta = os.path.join(os.path.join(first_fasta_output_dir, base + ".raxml.rba"))
      parse_result = parse_msa_info(first_run_log)
      cores = str(parse_result[0])
      taxa = str(parse_result[1])
      if (cores == "0" or taxa == "0"):
        print("warning with fasta " + fasta + ", skipping")
        continue
      writer.write("second_" + base + " ")
      writer.write(cores + " " + taxa )
      writer.write(" --msa " + compressed_fasta + " " + options[:-1])
      writer.write(" --prefix " + os.path.join(second_fasta_output_dir, base))
      writer.write(" --threads 1 ")
      writer.write("\n")
  return second_command_file

def main_raxml_runner(implementation, raxml_exec_dir, fasta_dir, output_dir, options_file, ranks):
  try:
    os.makedirs(output_dir)
  except:
    pass
  print("Results in " + output_dir)
  fasta_files = [os.path.join(fasta_dir, f) for f in os.listdir(fasta_dir)]
  options = open(options_file, "r").readlines()[0]
  raxml_library = os.path.join(raxml_exec_dir, "raxml-ng-mpi.so")
  first_command_file = build_first_command(raxml_exec_dir, fasta_files, output_dir, options, ranks)
  run_multiraxml(raxml_library, first_command_file, os.path.join(output_dir, "first_run"), ranks)
  print("### end of first multiraxml run")
  second_command_file = build_second_command(raxml_exec_dir, fasta_files, output_dir, options, ranks)
  print("### end of build_second_command")
  run_multiraxml(raxml_library, second_command_file, os.path.join(output_dir, "second_run"), ranks)
  print("### end of second multiraxml run")

def print_help():
  print("python raxml_runner.py --split-scheduler raxml_exec_dir fasta_dir output_dir additionnal_options_file cores_number")


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

