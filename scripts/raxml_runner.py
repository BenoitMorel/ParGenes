import sys
import os
import subprocess

def check_ranks(ranks):
  command = []
  command.append("mpirun")
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
  print("python raxml_runner.py raxml_exec_dir fasta_dir output_dir additionnal_options_file")

def get_first_run_output(output_dir):
  return os.path.join(output_dir, "first_run")

def run_multiraxml(command_filename, output_dir, ranks):
  command = []
  command.append("mpirun")
  command.append("-np")
  command.append("1")
  command.append(getMultiraxmlExec())
  command.append("--spawn-scheduler")
  command.append(command_filename)
  command.append(output_dir)
  command.append(str(ranks))
  subprocess.check_call(command)

def build_first_command(raxml_exec_dir, fasta_files, output_dir, options, ranks):
  first_command_file = os.path.join(output_dir, "first_command.txt")
  raxml = os.path.join(raxml_exec_dir, "raxml-ng")
  first_run_output_dir = get_first_run_output(output_dir)
  os.makedirs(first_run_output_dir)
  with open(first_command_file, "w") as writer:
    for fasta in fasta_files:
      base = os.path.splitext(os.path.basename(fasta))[0]
      writer.write("c_" + base + " nompi 1 1 " + raxml)
      writer.write(" --parse ")
      writer.write( " --msa " + fasta + " " + options[:-1])
      writer.write(" --prefix " + os.path.join(first_run_output_dir, base))
      writer.write(" --threads 1 ")
      writer.write("\n")
  return first_command_file

def main_raxml_runner(raxml_exec_dir, fasta_dir, output_dir, options_file, ranks):
  check_ranks(ranks)
  os.makedirs(output_dir)
  print("Results in " + output_dir)
  fasta_files = [os.path.join(fasta_dir, f) for f in os.listdir(fasta_dir)]
  options = open(options_file, "r").readlines()[0]
  first_command_file = build_first_command(raxml_exec_dir, fasta_files, output_dir, options, ranks)
  run_multiraxml(first_command_file, output_dir, ranks)

if (len(sys.argv) != 6):
    print_help()
    sys.exit(0)


raxml_exec_dir = sys.argv[1]
fasta_dir = sys.argv[2] 
output_dir = sys.argv[3]
options_file = sys.argv[4]
ranks = sys.argv[5]

main_raxml_runner(raxml_exec_dir, fasta_dir, output_dir, options_file, ranks)


