import sys
import os
import subprocess
import time
import shutil
import optparse

class MSA:
  name = ""
  path = ""
  compressed_path = ""
  valid = True
  taxa = 0
  sites = 0
  cores = 0
  model = ""
  raxml_arguments = ""
  raxml_additional_arguments = ""
  modeltest_additional_arguments = ""

  def __init__(self, name, path, raxml_arguments):
    self.name = name
    self.path = path
    self.valid = True
    self.raxml_arguments = raxml_arguments

  def set_model(self, model):
    self.model = model

  def set_model(self, model):
    self.model = model
    args = self.raxml_arguments.split(" ")
    for index, obj in enumerate(args):
      if (obj == "--model"):
        args[index + 1] = self.model
    self.raxml_arguments = " ".join(args)


def get_mpi_scheduler_exec():
  repo_root = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
  return os.path.join(repo_root, "mpi-scheduler", "build", "mpi-scheduler")

def sites_to_maxcores(sites):
  if sites == 0:
    return 0
  return 1 << ((sites // 1000)).bit_length()

def parse_msa_info(log_file, msa):
  unique_sites = 0
  try:
    lines = open(log_file).readlines()
  except:
    msa.valid = False
    return 
  for line in lines:
    if "Alignment comprises" in line:
      msa.sites = int(line.split(" ")[5])
    if "taxa" in line:
      msa.taxa = int(line.split(" ")[4])
  if (msa.sites * msa.taxa == 0):
    msa.valid = False
  msa.cores = sites_to_maxcores(msa.sites)

def run_mpi_scheduler(raxml_library, commands_filename, output_dir, ranks):
  sys.stdout.flush()
  command = []
  command.append("mpirun")
  command.append("-np")
  command.append(str(ranks))
  command.append(get_mpi_scheduler_exec())
  command.append("--split-scheduler")
  command.append(raxml_library)
  command.append(commands_filename)
  command.append(output_dir)
  print ("Calling mpi-scheduler: " + " ".join(command))
  subprocess.check_call(command)

def build_first_command(msas, output_dir, ranks):
  first_commands_file = os.path.join(output_dir, "first_command.txt")
  first_run_output_dir = os.path.join(output_dir, "first_run")
  first_run_results = os.path.join(first_run_output_dir, "results")
  os.makedirs(first_run_results)
  fasta_chuncks = []
  fasta_chuncks.append([])
  with open(first_commands_file, "w") as writer:
    for name, msa in msas.items():
      fasta_output_dir = os.path.join(first_run_results, name)
      os.makedirs(fasta_output_dir)
      writer.write("first_" + name + " 1 1 ")
      writer.write(" --parse ")
      writer.write( " --msa " + msa.path + " " + msa.raxml_arguments)
      writer.write(" --prefix " + os.path.join(fasta_output_dir, name))
      writer.write(" --threads 1 ")
      writer.write("\n")
  return first_commands_file
  
def analyse_parsed_msas(msas, output_dir):
  first_run_output_dir = os.path.join(output_dir, "first_run")
  first_run_results = os.path.join(first_run_output_dir, "results")
  for name, msa in msas.items():
    first_fasta_output_dir = os.path.join(first_run_results, name)
    first_run_log = os.path.join(os.path.join(first_fasta_output_dir, name + ".raxml.log"))
    parse_result = parse_msa_info(first_run_log, msa)
    if (not msa.valid):
      print("Warning, invalid MSA: " + name)
    msa.compressed_path = os.path.join(os.path.join(first_fasta_output_dir, name + ".raxml.rba"))

def build_modeltest_command(msas, output_dir, ranks): 
  modeltest_commands_file = os.path.join(output_dir, "modeltest_command.txt")
  modeltest_run_output_dir = os.path.join(output_dir, "modeltest_run")
  modeltest_results = os.path.join(modeltest_run_output_dir, "results")
  os.makedirs(modeltest_results)
  with open(modeltest_commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      modeltest_fasta_output_dir = os.path.join(modeltest_results, name)
      os.makedirs(modeltest_fasta_output_dir)
      writer.write("modeltest_" + name + " ") 
      writer.write("4 " + str(msa.taxa * msa.sites)) #todobenoit smarter ordering
      writer.write(" -i ")
      writer.write(msa.path)
      writer.write(" -t mp ")
      writer.write(" -o " +  os.path.join(modeltest_results, name, name))
      writer.write(" --template raxml ")
      writer.write("\n")
  return modeltest_commands_file

def parse_modeltest_results(msas, output_dir):
  modeltest_run_output_dir = os.path.join(output_dir, "modeltest_run")
  modeltest_results = os.path.join(modeltest_run_output_dir, "results")
  for name, msa in msas.items():
    if (not msa.valid):
      continue
    modeltest_outfile = os.path.join(modeltest_results, name, name + ".out")  
    with open(modeltest_outfile) as reader:
      read_next_model = False
      for line in reader.readlines():
        if (line.startswith("Best model according to AICc")):
            read_next_model = True
        if (read_next_model and line.startswith("Model")):
          msa.set_model(line.split(" ")[-1][:-1])
          break

def build_second_command(msas, output_dir, bootstraps, ranks):
  second_commands_file = os.path.join(output_dir, "second_command.txt")
  second_run_output_dir = os.path.join(output_dir, "second_run")
  second_run_results = os.path.join(second_run_output_dir, "results")
  second_run_bootstraps = os.path.join(second_run_output_dir, "bootstraps")
  os.makedirs(second_run_results)
  if (bootstraps != 0):
    os.makedirs(second_run_bootstraps)
  with open(second_commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      second_fasta_output_dir = os.path.join(second_run_results, name)
      os.makedirs(second_fasta_output_dir)
      writer.write("second_" + name + " ")
      writer.write(str(msa.cores) + " " + str(msa.taxa))
      writer.write(" --msa " + msa.compressed_path + " " + msa.raxml_arguments)
      writer.write(" --prefix " + os.path.join(second_fasta_output_dir, name))
      writer.write(" --threads 1 ")
      writer.write("\n")
      bs_output_dir = os.path.join(second_run_bootstraps, name)
      os.makedirs(bs_output_dir)
      chunk_size = 1
      if (bootstraps > 30): # arbitrary threshold... todobenoit!
        chunk_size = 10
      for current_bs in range(0, (bootstraps - 1) // chunk_size + 1):
        bsbase = name + "_bs" + str(current_bs)
        bs_number = min(chunk_size, bootstraps - current_bs * chunk_size)
        writer.write(bsbase + " ")
        writer.write(str(msa.cores) + " " + str(msa.taxa))
        writer.write(" --bootstrap")
        writer.write(" --msa " + msa.path + " " + msa.raxml_arguments)
        writer.write(" --prefix " + os.path.join(bs_output_dir, bsbase))
        writer.write(" --threads 1 ")
        writer.write(" --seed " + str(current_bs))
        writer.write(" --bs-trees " + str(bs_number))
        writer.write("\n")
  return second_commands_file

def concatenate_bootstraps(output_dir):
  start = time.time()
  print("concatenate_bootstraps")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  try:
    print("todobenoit remove the try catch")
    os.makedirs(concatenated_dir)
  except:
    pass
  bootstraps_dir = os.path.join(output_dir, "second_run", "bootstraps")
  for fasta in os.listdir(bootstraps_dir):
    concatenated_file = os.path.join(concatenated_dir, fasta + ".bs")
    with open(concatenated_file,'wb') as writer:
      fasta_bs_dir = os.path.join(bootstraps_dir, fasta)
      for bs_file in os.listdir(fasta_bs_dir):
        if (bs_file.endswith("bootstraps")):
          with open(os.path.join(fasta_bs_dir, bs_file),'rb') as reader:
            try:
              shutil.copyfileobj(reader, writer)
            except OSError as e:
              print("ERROR!")
              print("OS error when copying " + os.path.join(fasta_bs_dir, bs_file) + " to " + concatenated_file)
              raise e
              
  end = time.time()
  print("concatenation time: " + str(end-start) + "s")

def build_supports_commands(output_dir):
  ml_trees_dir = os.path.join(output_dir, "second_run", "results")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  supports_commands_file = os.path.join(output_dir, "supports_commands.txt")
  support_dir = os.path.join(output_dir, "supports_run")
  try:
    print("todobenoit remove try catch")
    os.makedirs(support_dir)
  except:
    pass
  print("Writing supports commands in " + supports_commands_file)
  with open(supports_commands_file, "w") as writer:
    for fasta in os.listdir(ml_trees_dir):
      ml_tree = os.path.join(ml_trees_dir, fasta, fasta + ".raxml.bestTree")
      bs_trees = os.path.join(concatenated_dir, fasta + ".bs")
      writer.write("support_" + fasta + " 1 1")
      writer.write(" --support")
      writer.write(" --tree " + ml_tree)
      writer.write(" --bs-trees " + bs_trees)
      writer.write(" --threads 1")
      writer.write(" --prefix " + os.path.join(support_dir, fasta + ".support"))
      writer.write("\n") 
  return supports_commands_file
    
def main_raxml_runner(op):
  output_dir = op.output_dir
  if (os.path.exists(output_dir)):
    print("[Error] The output directory " + output_dir + " already exists. Please use another output directory.")
    sys.exit(1)
  os.makedirs(output_dir)
  msas = {}
  scriptdir = os.path.dirname(os.path.realpath(__file__))
  raxml_library = os.path.join(scriptdir, "..", "raxml-ng", "bin", "raxml-ng-mpi.so")
  modeltest_library = os.path.join(scriptdir, "..", "modeltest", "build", "src", "modeltest-ng-mpi.so")
  print("Results in " + output_dir)
  options = open(op.raxml_global_parameters, "r").readlines()[0][:-1]
  for f in os.listdir(op.alignments_dir):
    name = os.path.splitext(f)[0]
    path = os.path.join(op.alignments_dir, f)
    msas[name] = MSA(name, path, options)
  first_commands_file = build_first_command(msas, output_dir, op.cores)
  run_mpi_scheduler(raxml_library, first_commands_file, os.path.join(output_dir, "first_run"), op.cores)
  print("### end of first mpi-scheduler run")
  analyse_parsed_msas(msas, output_dir)
  if (op.use_modeltest):
    modeltest_commands_file = build_modeltest_command(msas, output_dir, op.cores)
    run_mpi_scheduler(modeltest_library, modeltest_commands_file, os.path.join(output_dir, "modeltest_run"), op.cores)
    print("### end of modeltest mpi-scheduler run")
    parse_modeltest_results(msas, output_dir)
  second_commands_file = build_second_command(msas, output_dir, op.bootstraps, op.cores)
  run_mpi_scheduler(raxml_library, second_commands_file, os.path.join(output_dir, "second_run"), op.cores)
  print("### end of second mpi-scheduler run")
  if (op.bootstraps != 0):
    concatenate_bootstraps(output_dir)
    print("### end of bootstraps concatenation")
    supports_commands_file = build_supports_commands(output_dir)
    run_mpi_scheduler(raxml_library, supports_commands_file, os.path.join(output_dir, "supports_run"), op.cores)
    print("### end of supports mpi-scheduler run")

print("#################")
print("#  MULTI RAXML  #")
print("#################")
print("Multi-raxml was called as follow:")
print(" ".join(sys.argv))

parser = optparse.OptionParser()
parser.add_option('-a', "--alignments-dir", 
    dest="alignments_dir", 
    help="Directory containing the fasta files")
parser.add_option('-o', "--output-dir", 
    dest="output_dir", 
    help="Output directory")
parser.add_option("-r", "--raxml-global-parameters", 
    dest="raxml_global_parameters", 
    help="A file containing the parameters to pass to raxml")
#parser.add_option("--modeltest-global-parameters", 
#    dest="modeltest_global_parameters", 
#    help="A file containing the parameters to pass to modeltest")
parser.add_option("-b", "--bs-trees", 
    dest="bootstraps", 
    type=int,
    default=0,
    help="The number of bootstraps trees to compute")
parser.add_option("-c", "--cores", 
    dest="cores",
    type=int,
    help="The number of computational cores available for computation")
parser.add_option("-m", "--use-modeltest",
    dest="use_modeltest",
    action="store_true",
    default=False,
    help="Autodetect the model with modeltest")
#parser.add_option("--per-msa-raxml-arguments", 
#    dest="per_msa_raxml_arguments", 
#    help="A file containing per-msa raxml arguments")
#parser.add_option("--per-msa-modeltest-arguments", 
#    dest="per_msa_modeltest_arguments", 
#    help="A file containing per-msa modeltest arguments")

op, remainder = parser.parse_args()

start = time.time()
main_raxml_runner(op)
end = time.time()
print("TOTAL ELAPSED TIME SPENT IN " + os.path.basename(__file__) + " " + str(end-start) + "s")

