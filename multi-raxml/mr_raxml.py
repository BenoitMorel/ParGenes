import os
import mr_commons
import shutil
import mr_scheduler
import concurrent.futures




def parse_msa_info(log_file, msa, core_assignment):
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
    elif "Alignment sites / patterns" in line:
      msa.patterns = int(line.split(" ")[6])
    elif "Per-taxon CLV size" in line:
      msa.per_taxon_clv_size = int(line.split(" : ")[1])
    # find the number of cores depending on the selected policy 
    if (core_assignment == "high"):
      if "minimum response time" in line:
        msa.cores = int(line.split(" : ")[1])
    elif (core_assignment == "medium"):
      if "minimum response time" in line:
        msa.cores = int(line.split(" : ")[1])
    elif (core_assignment == "low"):
      if "" in line:
        msa.cores = int(line.split(" : ")[1])
    else:
      print("ERROR: unknown core_assignment " + core_assignment)
      sys.exit(1)
  if (msa.per_taxon_clv_size * msa.taxa == 0):
    msa.valid = False

def improve_cores_assignment(msas, op):
  average_taxa = 0
  max_taxa = 0
  average_sites = 0
  max_sites = 0
  taxa_numbers = []
  for name, msa in msas.items():
    taxa_numbers.append(msa.taxa)
    average_taxa += msa.taxa
    average_sites += msa.patterns
    max_taxa = max(max_taxa, msa.taxa)
    max_sites = max(max_sites, msa.patterns)
  taxa_numbers.sort()
  average_taxa /= len(msas)
  average_sites /= len(msas)
  print("Average number of taxa: " + str(average_taxa))
  print("Max number of taxa: " + str(max_taxa))
  print("Average number of sites: " + str(average_sites))
  print("Max number of sites: " + str(max_sites))
  if (op.percentage_jobs_double_cores > 0.0):
    ratio = 1.0 - op.percentage_jobs_double_cores
    limit_taxa = taxa_numbers[int(float(len(msas)) * ratio)]
    print("Limit taxa: " + str(limit_taxa))
    for name, msa in msas.items():
      if (msa.taxa < limit_taxa):
        if (msa.cores > 1):
          msa.cores = msa.cores // 2

def  predict_number_cores(msas, op):
  msa_count = len(msas)
  total_cost = 0
  worst_percpu_cost = 0
  runs_number = op.parsimony_starting_trees + op.random_starting_trees + op.bootstraps
  for name, msa in msas.items():
    if (not msa.valid):
      continue
    total_cost += msa.taxa * msa.per_taxon_clv_size
    worst_percpu_cost = max(worst_percpu_cost, (msa.taxa * msa.per_taxon_clv_size) // msa.cores)
  print("Total cost for one ML search: " + str(total_cost))
  print("Worst per cpu cost for one ML search: " + str(worst_percpu_cost))
  print("Number of ML searches per MSA: " + str(runs_number))
  cores = total_cost // worst_percpu_cost
  cores *= runs_number
  print("Recommended number of cores: " + str(max(1, cores // 4)))
  print("DISCLAIMER!!! Please note this number is a rough estimate only, and can differ on your system.")

def run_parsing_step(msas, library, scheduler, parse_run_output_dir, cores, op):
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
  mr_scheduler.run_mpi_scheduler(library, scheduler, parse_commands_file, parse_run_output_dir, cores, op)  
 
def analyse_parsed_msas(msas, op, output_dir):
  """ Analyse results from run_parsing_step and store them into msas """
  core_assignment = op.core_assignment
  parse_run_output_dir = os.path.join(output_dir, "parse_run")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  invalid_msas = []
  print("analyse_parsed_msas")
  count = 0
  for name, msa in msas.items():
    count += 1
    parse_fasta_output_dir = os.path.join(parse_run_results, name)
    parse_run_log = os.path.join(os.path.join(parse_fasta_output_dir, name + ".raxml.log"))
    parse_result = parse_msa_info(parse_run_log, msa, core_assignment)
    if (not msa.valid):
      invalid_msas.append(msa)      
  improve_cores_assignment(msas, op)
  predict_number_cores(msas, op)
  if (len(invalid_msas) > 0):
    invalid_msas_file = os.path.join(output_dir, "invalid_msas.txt")
    print("[Warning] Found " + str(len(invalid_msas)) + " invalid MSAs (see " + invalid_msas_file + ")") 
    with open(invalid_msas_file, "w") as f:
      for msa in invalid_msas:
        f.write(msa.name + "\n")

def run(msas, random_trees, parsimony_trees, bootstraps, library, scheduler, run_path, cores, op):
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
        msa_size = msa.taxa * msa.per_taxon_clv_size
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
      chunk_size = 10
      for current_bs in range(0, (bootstraps - 1) // chunk_size + 1):
        bsbase = name + "_bs" + str(current_bs)
        bs_number = min(chunk_size, bootstraps - current_bs * chunk_size)
        writer.write(bsbase + " ")
        writer.write(str((msa.cores // 2) * chunk_size) + " " + str(msa_size))
        writer.write(" --bootstrap")
        writer.write(" --msa " + msa.path + " " + msa.raxml_arguments)
        writer.write(" --prefix " + os.path.join(bs_output_dir, bsbase))
        writer.write(" --threads 1 ")
        writer.write(" --seed " + str(current_bs + 1))
        writer.write(" --bs-trees " + str(bs_number))
        writer.write("\n")
  mr_scheduler.run_mpi_scheduler(library, scheduler, commands_file, run_path, cores, op)  


def extract_ll_from_raxml_logs(raxml_log_file):
  """ Return the final likelihood from a raxml log file """
  res = -float('inf')
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
  with concurrent.futures.ThreadPoolExecutor(max_workers = min(16, int(op.cores))) as e:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      e.submit(select_best_ml_tree_msa, msa, results_path, op) 

