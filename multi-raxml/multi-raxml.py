import sys
import os
import subprocess
import time
import shutil
import argparse
import glob

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
  modeltest_arguments = ""

  def __init__(self, name, path, raxml_arguments, modeltest_arguments):
    self.name = name
    self.path = path
    self.valid = True
    self.raxml_arguments = raxml_arguments
    self.modeltest_arguments = modeltest_arguments

  def set_model(self, model):
    self.model = model

  def set_model(self, model):
    self.model = model
    args = self.raxml_arguments.split(" ")
    for index, obj in enumerate(args):
      if (obj == "--model"):
        args[index + 1] = self.model
    self.raxml_arguments = " ".join(args)

def get_msa_name(msa_file):
  return msa_file.replace(".", "_")

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
  logs_file = os.path.join(output_dir, "logs.txt")
  out = open(logs_file, "w")
  print("Calling mpi-scheduler: " + " ".join(command))
  print("Logs will be redirected to " + logs_file)
  p = subprocess.Popen(command, stdout=out, stderr=out)
  p.wait()

def build_parse_command(msas, output_dir, ranks):
  parse_commands_file = os.path.join(output_dir, "parse_command.txt")
  parse_run_output_dir = os.path.join(output_dir, "parse_run")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  os.makedirs(parse_run_results)
  fasta_chuncks = []
  fasta_chuncks.append([])
  with open(parse_commands_file, "w") as writer:
    for name, msa in msas.items():
      fasta_output_dir = os.path.join(parse_run_results, name)
      os.makedirs(fasta_output_dir)
      writer.write("parse_" + name + " 1 1 ")
      writer.write(" --parse ")
      writer.write( " --msa " + msa.path + " " + msa.raxml_arguments)
      writer.write(" --prefix " + os.path.join(fasta_output_dir, name))
      writer.write(" --threads 1 ")
      writer.write("\n")
  return parse_commands_file
  
def analyse_parsed_msas(msas, output_dir):
  parse_run_output_dir = os.path.join(output_dir, "parse_run")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  for name, msa in msas.items():
    parse_fasta_output_dir = os.path.join(parse_run_results, name)
    parse_run_log = os.path.join(os.path.join(parse_fasta_output_dir, name + ".raxml.log"))
    parse_result = parse_msa_info(parse_run_log, msa)
    if (not msa.valid):
      print("Warning, invalid MSA: " + name)
    msa.compressed_path = os.path.join(os.path.join(parse_fasta_output_dir, name + ".raxml.rba"))

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
      writer.write(" " + msa.modeltest_arguments + " ")
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

def build_mlsearch_command(msas, output_dir, starting_trees, bootstraps, ranks):
  mlsearch_commands_file = os.path.join(output_dir, "mlsearch_command.txt")
  mlsearch_run_output_dir = os.path.join(output_dir, "mlsearch_run")
  mlsearch_run_results = os.path.join(mlsearch_run_output_dir, "results")
  mlsearch_run_bootstraps = os.path.join(mlsearch_run_output_dir, "bootstraps")
  os.makedirs(mlsearch_run_results)
  if (bootstraps != 0):
    os.makedirs(mlsearch_run_bootstraps)
  with open(mlsearch_commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      mlsearch_fasta_output_dir = os.path.join(mlsearch_run_results, name)
      os.makedirs(mlsearch_fasta_output_dir)
      for starting_tree in range(0, starting_trees):
        if (starting_trees > 1):
          prefix = os.path.join(mlsearch_fasta_output_dir, "multiple_runs", str(starting_tree))
          os.makedirs(prefix)
          prefix = os.path.join(prefix, name)
        else:
          prefix = os.path.join(mlsearch_fasta_output_dir, name)
        writer.write("mlsearch_" + name + "_" + str(starting_tree) + " ")
        writer.write(str(msa.cores) + " " + str(msa.taxa))
        writer.write(" --msa " + msa.compressed_path + " " + msa.raxml_arguments)
        writer.write(" --prefix " + prefix)
        writer.write(" --threads 1 ")
        if (starting_trees > 1):
          writer.write(" --seed " + str(starting_tree) + " ")
        writer.write("\n")
      bs_output_dir = os.path.join(mlsearch_run_bootstraps, name)
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
  return mlsearch_commands_file

def concatenate_bootstraps(output_dir):
  start = time.time()
  print("concatenate_bootstraps")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  try:
    print("todobenoit remove the try catch")
    os.makedirs(concatenated_dir)
  except:
    pass
  bootstraps_dir = os.path.join(output_dir, "mlsearch_run", "bootstraps")
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
  ml_trees_dir = os.path.join(output_dir, "mlsearch_run", "results")
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
   
def get_filter_content(msa_filter):
  if (msa_filter == None):
    return None
  msas_to_process = {}
  with open(msa_filter, "r") as reader:
    for line in reader.readlines():
      msa = line[:-1]
      msas_to_process[msa] = msa
  return msas_to_process

def add_per_msa_raxml_options(msas, options_file):
  if (options_file == None):
    return
  per_msa_options = {}
  with open(options_file, "r") as reader:
    for line in reader.readlines():
      split = line[:-1].split(" ")
      msa = get_msa_name(split[0])
      if (not msa in msas):
        print("[Warning] Found msa " + msa + " in options file " + options_file + " but not in the msas directory")
        continue
      msas[msa].raxml_arguments += " " + " ".join(split[1:])

def add_per_msa_modeltest_options(msas, options_file):
  if (options_file == None):
    return
  per_msa_options = {}
  with open(options_file, "r") as reader:
    for line in reader.readlines():
      split = line[:-1].split(" ")
      msa = get_msa_name(split[0])
      if (not msa in msas):
        print("[Warning] Found msa " + msa + " in options file " + options_file + " but not in the msas directory")
        continue
      msas[msa].modeltest_arguments += " " + " ".join(split[1:])

def init_msas(op):
  msas = {}
  raxml_options = ""
  modeltest_options = ""
  if (op.raxml_global_parameters != None):
    raxml_options = open(op.raxml_global_parameters, "r").readlines()[0][:-1]
  if (op.raxml_global_parameters_string != None):
    raxml_options += " " + op.raxml_global_parameters_string
  if (op.modeltest_global_parameters != None):
    modeltest_options = open(op.modeltest_global_parameters, "r").readlines()[0][:-1]
  msa_filter = get_filter_content(op.msa_filter)
  for f in os.listdir(op.alignments_dir):
    if (msa_filter != None):
      if (not (f in msa_filter)):
        continue
      del msa_filter[f]
    path = os.path.join(op.alignments_dir, f)
    name = get_msa_name(f)
    msas[name] = MSA(name, path, raxml_options, modeltest_options)
  if (msa_filter != None): # check that all files in the filter are present in the directory
    for msa in msa_filter.values():
      print("[Warning] File " + msa + " was found in the filter file, but not in the MSAs directory")
  add_per_msa_raxml_options(msas, op.per_msa_raxml_parameters)
  add_per_msa_modeltest_options(msas, op.per_msa_modeltest_parameters)
  return msas

def extract_ll_from_raxml_logs(raxml_log_file):
  return 0.0

def select_best_ml_tree(msas, op):
  results_path = os.path.join(op.output_dir, "mlsearch_run", "results")
  for name, msa in msas.items():
    msa_results_path = os.path.join(results_path, name)
    msa_multiple_results_path = os.path.join(msa_results_path, "multiple_runs")
    best_ll = -float('inf')
    best_starting_tree = 0
    for starting_tree in range(0, op.starting_trees):
      raxml_logs = os.path.join(msa_multiple_results_path, str(starting_tree), name + ".raxml.log")
      ll = extract_ll_from_raxml_logs(raxml_logs)
      if (ll > best_ll):
        best_ll = ll
        best_starting_tree = starting_tree
    directory_to_copy = os.path.join(msa_multiple_results_path, str(best_starting_tree))
    files_to_copy = os.listdir(directory_to_copy)
    for f in files_to_copy:
      shutil.copy(os.path.join(directory_to_copy, f), msa_results_path)

def main_raxml_runner(op):
  output_dir = op.output_dir
  if (os.path.exists(output_dir)):
    print("[Error] The output directory " + output_dir + " already exists. Please use another output directory.")
    sys.exit(1)
  os.makedirs(output_dir)
  print("Results in " + output_dir)
  msas = init_msas(op)
  scriptdir = os.path.dirname(os.path.realpath(__file__))
  raxml_library = os.path.join(scriptdir, "..", "raxml-ng", "bin", "raxml-ng-mpi.so")
  modeltest_library = os.path.join(scriptdir, "..", "modeltest", "build", "src", "modeltest-ng-mpi.so")
  parse_commands_file = build_parse_command(msas, output_dir, op.cores)
  run_mpi_scheduler(raxml_library, parse_commands_file, os.path.join(output_dir, "parse_run"), op.cores)
  print("### end of parsing mpi-scheduler run")
  analyse_parsed_msas(msas, output_dir)
  if (op.use_modeltest):
    modeltest_commands_file = build_modeltest_command(msas, output_dir, op.cores)
    run_mpi_scheduler(modeltest_library, modeltest_commands_file, os.path.join(output_dir, "modeltest_run"), op.cores)
    print("### end of modeltest mpi-scheduler run")
    parse_modeltest_results(msas, output_dir)
  mlsearch_commands_file = build_mlsearch_command(msas, output_dir, op.starting_trees, op.bootstraps, op.cores)
  run_mpi_scheduler(raxml_library, mlsearch_commands_file, os.path.join(output_dir, "mlsearch_run"), op.cores)
  if (op.starting_trees > 1):
    select_best_ml_tree(msas, op)
  print("### end of mlsearch mpi-scheduler run")
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

parser = argparse.ArgumentParser()
parser.add_argument('-a', "--alignments-dir", 
    dest="alignments_dir", 
    help="Directory containing the fasta files")
parser.add_argument('-o', "--output-dir", 
    dest="output_dir", 
    help="Output directory")
parser.add_argument("-r", "--raxml-global-parameters", 
    dest="raxml_global_parameters", 
    help="A file containing the parameters to pass to raxml")
parser.add_argument("-R", "--raxml-global-parameters-string", 
    dest="raxml_global_parameters_string", 
    help="List of parameters to pass to raxml (see also --raxml-global-parameters)")
parser.add_argument("--modeltest-global-parameters", 
    dest="modeltest_global_parameters", 
    help="A file containing the parameters to pass to modeltest")
parser.add_argument("-b", "--bs-trees", 
    dest="bootstraps", 
    type=int,
    default=0,
    help="The number of bootstrap trees to compute")
parser.add_argument("-s", "--raxml-starting-trees", 
    dest="starting_trees", 
    type=int,
    default=1,
    help="The number of starting trees")
parser.add_argument("-c", "--cores", 
    dest="cores",
    type=int,
    help="The number of computational cores available for computation")
parser.add_argument("-m", "--use-modeltest",
    dest="use_modeltest",
    action="store_true",
    default=False,
    help="Autodetect the model with modeltest")
parser.add_argument("--msa-filter",
    dest="msa_filter", 
    help="A file containing the names of the msa files to process")
parser.add_argument("--per-msa-raxml-parameters", 
    dest="per_msa_raxml_parameters", 
    help="A file containing per-msa raxml parameters")
parser.add_argument("--per-msa-modeltest-parameters",
    dest="per_msa_modeltest_parameters", 
    help="A file containing per-msa modeltest parameters")

op = parser.parse_args()

start = time.time()
main_raxml_runner(op)
end = time.time()
print("TOTAL ELAPSED TIME SPENT IN " + os.path.basename(__file__) + " " + str(end-start) + "s")

