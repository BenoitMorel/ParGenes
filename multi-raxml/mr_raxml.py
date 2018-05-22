import os
import mr_commons
import shutil
import mr_scheduler
import concurrent.futures




def parse_msa_info(log_file, msa):
  """ Parse the raxml log_file and store the number of 
  taxa and unique sites in msa. Flag invalid msas to invalid  """
  unique_sites = 0
  try:
    lines = open(log_file).readlines()
  except:
    msa.valid = False
    return 
  for line in lines:
    if "taxa" in line:
      msa.taxa = int(line.split(" ")[4])
    if "minimum response time" in line:
      msa.cores = int(line.split(" : ")[1])
    if "Per-taxon CLV size" in line:
      msa.sites = int(line.split(" : ")[1])
  if (msa.sites * msa.taxa == 0):
    msa.valid = False

def run_parsing_step(msas, library, scheduler, parse_run_output_dir, cores):
  """ Run raxml-ng --parse on each MSA to check it is valid and
  to get its MSA dimensiosn """
  parse_commands_file = os.path.join(parse_run_output_dir, "parse_command.txt")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  mr_commons.makedirs(parse_run_results)
  fasta_chuncks = []
  fasta_chuncks.append([])
  with open(parse_commands_file, "w") as writer:
    for name, msa in msas.items():
      fasta_output_dir = os.path.join(parse_run_results, name)
      mr_commons.makedirs(fasta_output_dir)
      writer.write("parse_" + name + " 1 1 ")
      writer.write(" --parse ")
      writer.write(" --log DEBUG ")
      writer.write( " --msa " + msa.path + " " + msa.raxml_arguments)
      writer.write(" --prefix " + os.path.join(fasta_output_dir, name))
      writer.write(" --threads 1 ")
      writer.write("\n")
  mr_scheduler.run_mpi_scheduler(library, scheduler, parse_commands_file, parse_run_output_dir, cores)  
 
def analyse_parsed_msas(msas, output_dir):
  """ Analyse results from run_parsing_step and store them into msas """
  parse_run_output_dir = os.path.join(output_dir, "parse_run")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  invalid_msas = []
  print("analyse_parsed_msas")
  count = 0
  for name, msa in msas.items():
    if (count % 500 == 0):
      print("analysed " + str(count) + " msas")
    count += 1
    parse_fasta_output_dir = os.path.join(parse_run_results, name)
    parse_run_log = os.path.join(os.path.join(parse_fasta_output_dir, name + ".raxml.log"))
    parse_result = parse_msa_info(parse_run_log, msa)
    if (not msa.valid):
      invalid_msas.append(msa)      

  if (len(invalid_msas) > 0):
    invalid_msas_file = os.path.join(output_dir, "invalid_msas.txt")
    print("[Warning] Found " + str(len(invalid_msas)) + " invalid MSAs (see " + invalid_msas_file + ")") 
    with open(invalid_msas_file, "w") as f:
      for msa in invalid_msas:
        f.write(msa.name + "\n")

def run(msas, random_trees, parsimony_trees, bootstraps, library, scheduler, run_path, cores):
  """ Use the MPI scheduler to run raxml-ng on all the dataset. 
  Also schedules the bootstraps runs"""
  commands_file = os.path.join(run_path, "mlsearch_command.txt")
  mlsearch_run_results = os.path.join(run_path, "results")
  mlsearch_run_bootstraps = os.path.join(run_path, "bootstraps")
  mr_commons.makedirs(mlsearch_run_results)
  starting_trees = random_trees + parsimony_trees
  if (bootstraps != 0):
    mr_commons.makedirs(mlsearch_run_bootstraps)
  with open(commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      msa_size = 1
      if (not msa.flag_disable_sorting):
        msa_size = msa.taxa * msa.sites
      mlsearch_fasta_output_dir = os.path.join(mlsearch_run_results, name)
      mr_commons.makedirs(mlsearch_fasta_output_dir)
      for starting_tree in range(0, starting_trees):
        if (starting_trees > 1):
          prefix = os.path.join(mlsearch_fasta_output_dir, "multiple_runs", str(starting_tree))
          mr_commons.makedirs(prefix)
          prefix = os.path.join(prefix, name)
        else:
          prefix = os.path.join(mlsearch_fasta_output_dir, name)
        writer.write("mlsearch_" + name + "_" + str(starting_tree) + " ")
        writer.write(str(msa.cores) + " " + str(msa_size))
        writer.write(" --msa " + msa.path + " " + msa.raxml_arguments)
        writer.write(" --prefix " + prefix)
        writer.write(" --threads 1 ")
        if (starting_tree >= random_trees):
          writer.write(" --tree pars ")
        writer.write(" --seed " + str(starting_tree + 1) + " ")
        writer.write("\n")
      bs_output_dir = os.path.join(mlsearch_run_bootstraps, name)
      mr_commons.makedirs(bs_output_dir)
      chunk_size = 1
      if (bootstraps > 30): # arbitrary threshold... todobenoit!
        chunk_size = 10
      for current_bs in range(0, (bootstraps - 1) // chunk_size + 1):
        bsbase = name + "_bs" + str(current_bs)
        bs_number = min(chunk_size, bootstraps - current_bs * chunk_size)
        writer.write(bsbase + " ")
        writer.write(str(msa.cores) + " " + str(msa_size))
        writer.write(" --bootstrap")
        writer.write(" --msa " + msa.path + " " + msa.raxml_arguments)
        writer.write(" --prefix " + os.path.join(bs_output_dir, bsbase))
        writer.write(" --threads 1 ")
        writer.write(" --seed " + str(current_bs + 1))
        writer.write(" --bs-trees " + str(bs_number))
        writer.write("\n")
  mr_scheduler.run_mpi_scheduler(library, scheduler, commands_file, run_path, cores)  


def extract_ll_from_raxml_logs(raxml_log_file):
  """ Return the final likelihood from a raxml log file """
  res = 0.0
  with open(raxml_log_file) as reader:
    for line in reader.readlines():
      if (line.startswith("Final LogLikelihood:")):
        res = float(line.split(" ")[2][:-1])
  return res

def select_best_ml_tree_msa(msa, results_path, op):
  """ Find the raxml run that got the best likelihood, 
  and copy its output files in the results directory """
  name = msa.name
  msa_results_path = os.path.join(results_path, name)
  msa_multiple_results_path = os.path.join(msa_results_path, "multiple_runs")
  best_ll = -float('inf')
  best_starting_tree = 0
  for starting_tree in range(0, op.random_starting_trees + op.parsimony_starting_trees):
    raxml_logs = os.path.join(msa_multiple_results_path, str(starting_tree), name + ".raxml.log")
    ll = extract_ll_from_raxml_logs(raxml_logs)
    if (ll > best_ll):
      best_ll = ll
      best_starting_tree = starting_tree
  directory_to_copy = os.path.join(msa_multiple_results_path, str(best_starting_tree))
  files_to_copy = os.listdir(directory_to_copy)
  for f in files_to_copy:
    shutil.copy(os.path.join(directory_to_copy, f), msa_results_path)



def select_best_ml_tree(msas, op):
  """ Run select_best_ml_tree_msa in parallel (one one single node) on all the MSAs """
  results_path = os.path.join(op.output_dir, "mlsearch_run", "results")
  with concurrent.futures.ThreadPoolExecutor() as e:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      e.submit(select_best_ml_tree_msa, msa, results_path, op) 

