import os
import commons
import shutil
import scheduler
import multiprocessing
import pickle
import logger
import constraint

def parse_msa_info(log_file, msa, core_assignment):
  """ Parse the raxml log_file and store the number of 
  taxa and unique sites in msa. Flag invalid msas to invalid  """
  unique_sites = 0
  try:
    lines = open(log_file).readlines()
  except:
    msa.valid = False
    return 
  msa.cores = 0
  for line in lines:
    line = line[:-1]
    if ("Loaded alignment") in line and ("taxa" in line):
      msa.taxa = int(line.split(" ")[4])
    elif "Alignment sites / patterns" in line:
      msa.patterns = int(line.split(" ")[6])
    elif "Per-taxon CLV size" in line:
      msa.per_taxon_clv_size = int(line.split(" : ")[1])
    # find the number of cores depending on the selected policy 
    if (core_assignment == "high"):
      if "Maximum     number of threads / MPI processes" in line:
        msa.cores = int(line.split(": ")[1])
    elif (core_assignment == "medium"):
      if "Recommended number of threads / MPI processes" in line:
        msa.cores = int(line.split(": ")[1])
    elif (core_assignment == "low"):
      if "Minimum     number of threads / MPI processes" in line:
        msa.cores = int(line.split(": ")[1])
    else:
      logger.error("Unknown core_assignment " + core_assignment)
  if (msa.per_taxon_clv_size * msa.taxa * msa.cores == 0):
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
  if (len(msas) > 0):
    average_taxa /= len(msas)
    average_sites /= len(msas)
  logger.info("  Number of families: " + str(len(msas)))
  logger.info("  Average number of taxa: " + str(int(average_taxa)))
  logger.info("  Max number of taxa: " + str(max_taxa))
  logger.info("  Average number of sites: " + str(int(average_sites)))
  logger.info("  Max number of sites: " + str(max_sites))
  if (op.percentage_jobs_double_cores > 0.0):
    ratio = 1.0 - op.percentage_jobs_double_cores
    limit_taxa = taxa_numbers[int(float(len(msas)) * ratio)]
    for name, msa in msas.items():
      if (msa.taxa < limit_taxa):
        if (msa.cores > 1):
          msa.cores = msa.cores // 2

def  predict_number_cores(msas, op):
  msa_count = len(msas)
  total_cost = 0
  worst_percpu_cost = 1
  runs_number = op.parsimony_starting_trees + op.random_starting_trees + op.bootstraps
  for name, msa in msas.items():
    if (not msa.valid):
      continue
    total_cost += msa.taxa * msa.per_taxon_clv_size
    worst_percpu_cost = max(worst_percpu_cost, (msa.taxa * msa.per_taxon_clv_size) // msa.cores)
  cores = total_cost // worst_percpu_cost
  cores *= runs_number
  logger.info("  Recommended MAXIMUM number of cores: " + str(max(1, cores // 4)))

def run_parsing_step(msas, library, scheduler_mode, parse_run_output_dir, cores, op):
  """ Run raxml-ng --parse on each MSA to check it is valid and
  to get its MSA dimensiosn """
  parse_commands_file = os.path.join(parse_run_output_dir, "parse_command.txt")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  commons.makedirs(parse_run_results)
  fasta_chuncks = []
  fasta_chuncks.append([])
  with open(parse_commands_file, "w") as writer:
    for name, msa in msas.items():
      fasta_output_dir = os.path.join(parse_run_results, name)
      commons.makedirs(fasta_output_dir)
      if (op.use_modeltest):
        if (not msa.has_model()):
          # set a fake model to make raxml parsing happy
          # if will be replaced after modeltest run
          if (op.datatype == "aa"):
            msa.set_model("WAG")
          else:
            msa.set_model("GTR")
      writer.write("parse_" + name + " 1 1 ")
      writer.write(" --parse ")
      writer.write(" --log DEBUG ")
      writer.write( " --msa " + msa.path + " " + msa.get_raxml_arguments_str())
      writer.write(" --prefix " + os.path.join(fasta_output_dir, name))
      if (scheduler_mode != "fork"):
        writer.write(" --threads 1 ")
        

      writer.write("\n")
  scheduler.run_scheduler(library, scheduler_mode, "--threads", parse_commands_file, parse_run_output_dir, cores, op)  
 
def analyse_parsed_msas(msas, op):
  """ Analyse results from run_parsing_step and store them into msas """
  output_dir = op.output_dir
  core_assignment = op.core_assignment
  parse_run_output_dir = os.path.join(output_dir, "parse_run")
  parse_run_results = os.path.join(parse_run_output_dir, "results")
  invalid_msas = []
  count = 0
  for name, msa in msas.items():
    count += 1
    parse_fasta_output_dir = os.path.join(parse_run_results, name)
    parse_run_log = os.path.join(os.path.join(parse_fasta_output_dir, name + ".raxml.log"))
    msa.binary_path = os.path.join(os.path.join(parse_fasta_output_dir, name + ".raxml.rba"))
    parse_result = parse_msa_info(parse_run_log, msa, core_assignment)
    if (not msa.valid):
      invalid_msas.append(msa)      
  improve_cores_assignment(msas, op)
  predict_number_cores(msas, op)
  if (len(invalid_msas) > 0):
    invalid_msas_file = os.path.join(output_dir, "invalid_msas.txt")
    logger.warning("Found " + str(len(invalid_msas)) + " invalid MSAs (see " + invalid_msas_file + ")") 
    with open(invalid_msas_file, "w") as f:
      for msa in invalid_msas:
        f.write(msa.name + "\n")
  save_msas(msas, op)

def save_msas(msas, op):
  with open(os.path.join(op.output_dir, "parse_run", "msas_checkpoint.bin"), "ab") as f:
    pickle.dump(msas, f, pickle.HIGHEST_PROTOCOL)      

def load_msas(op):
  with open(os.path.join(op.output_dir, "parse_run", "msas_checkpoint.bin"), "rb") as f:
    return pickle.load(f)
  

def run(msas, random_trees, parsimony_trees, bootstraps, library, scheduler_mode, run_path, cores, op):
  """ Use the MPI scheduler_mode to run raxml-ng on all the dataset. 
  Also schedules the bootstraps runs"""
  commands_file = os.path.join(run_path, "mlsearch_command.txt")
  mlsearch_run_results = os.path.join(run_path, "results")
  mlsearch_run_bootstraps = os.path.join(run_path, "bootstraps")
  commons.makedirs(mlsearch_run_results)
  starting_trees = random_trees + parsimony_trees
  chunk_size = 2
  msa_number = len(msas)
  if (msa_number < 50):
    chunk_size = 1

  if (bootstraps != 0):
    commons.makedirs(mlsearch_run_bootstraps)
  with open(commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      msa_path = msa.binary_path
      if (msa_path == "" or msa_path == None):
        msa_path = msa.path
      msa_size = 1
      if (not msa.flag_disable_sorting):
        msa_size = msa.taxa * msa.per_taxon_clv_size
      mlsearch_fasta_output_dir = os.path.join(mlsearch_run_results, name)
      commons.makedirs(mlsearch_fasta_output_dir)
      # generate all the mlsearch commands
      for starting_tree in range(0, starting_trees):
        if (starting_trees > 1):
          prefix = os.path.join(mlsearch_fasta_output_dir, "multiple_runs", str(starting_tree))
          commons.makedirs(prefix)
          prefix = os.path.join(prefix, name)
        else:
          prefix = os.path.join(mlsearch_fasta_output_dir, name)
        writer.write("mlsearch_" + name + "_" + str(starting_tree) + " ")
        writer.write(str(msa.cores) + " " + str(msa_size))
        writer.write(" --msa " + msa_path + " " + msa.get_raxml_arguments_str())
        writer.write(" --prefix " + os.path.abspath(prefix))
        if (scheduler_mode != "fork"):
          writer.write(" --threads 1 ")
        if (starting_tree >= random_trees):
          writer.write(" --tree pars{1} ")
        else:
          writer.write(" --tree rand{1} ")
        if (op.constrain_search):
          writer.write(" --tree-constraint " + constraint.get_constraint(op, name))
          writer.write(" --force")
        writer.write(" --seed " + str(starting_tree + op.seed + 1) + " ")
        writer.write("\n")
      bs_output_dir = os.path.join(mlsearch_run_bootstraps, name)
      commons.makedirs(bs_output_dir)
      # generate all the boostrap commands
      per_family_bootstrap_runs = (bootstraps - 1) // chunk_size + 1
      if (op.autoMRE):
        per_family_bootstrap_runs = 1
      for current_bs in range(0, per_family_bootstrap_runs):
        bsbase = name + "_bs" + str(current_bs)
        writer.write(bsbase + " ")
        writer.write(str(max(1, msa.cores // 2)) + " " + str(msa_size * chunk_size))
        writer.write(" --bootstrap")
        writer.write(" --msa " + msa_path + " " + msa.get_raxml_arguments_str())
        writer.write(" --prefix " + os.path.abspath(os.path.join(bs_output_dir, bsbase)))
        if (scheduler_mode != "fork"):
          writer.write(" --threads 1 ")
        writer.write(" --seed " + str(current_bs + op.seed + 1))
        if (not op.autoMRE):
          bs_number = min(chunk_size, bootstraps - current_bs * chunk_size)
          writer.write(" --bs-trees " + str(bs_number))
        else:
          writer.write(" --bs-trees autoMRE{" + str(bootstraps) + "}")
        writer.write("\n")
  scheduler.run_scheduler(library, scheduler_mode, "--threads", commands_file, run_path, cores, op)  


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
  all_ml_trees = os.path.join(msa_results_path, "sorted_ml_trees.newick")
  all_ml_trees_ll = os.path.join(msa_results_path, "sorted_ml_trees_ll.newick")
  best_ll = -float('inf')
  best_starting_tree = 0
  ll_and_trees = []
  for starting_tree in range(0, op.random_starting_trees + op.parsimony_starting_trees):
    raxml_logs = os.path.join(msa_multiple_results_path, str(starting_tree), name + ".raxml.log")
    ll = extract_ll_from_raxml_logs(raxml_logs)
    ll_and_trees.append((ll, starting_tree))
    if (ll > best_ll):
      best_ll = ll
      best_starting_tree = starting_tree
  #logger.info(ll_and_tree)
  directory_to_copy = os.path.join(msa_multiple_results_path, str(best_starting_tree))
  files_to_copy = os.listdir(directory_to_copy)
  for f in files_to_copy:
    shutil.copy(os.path.join(directory_to_copy, f), msa_results_path)
  
  with open(all_ml_trees, "w") as writer:
    for tree in sorted(ll_and_trees, key=lambda x: x[0], reverse=True):
      tree_file = os.path.join(msa_multiple_results_path, str(tree[1]), name + ".raxml.bestTree")
      writer.write(open(tree_file).read())
  with open(all_ml_trees_ll, "w") as writer:
    for tree in sorted(ll_and_trees, key=lambda x: x[0], reverse=True):
      tree_file = os.path.join(msa_multiple_results_path, str(tree[1]), name + ".raxml.bestTree")
      writer.write(str(tree[0]) + " ")
      writer.write(open(tree_file).read())



def select_best_ml_tree(msas, op):
  """ Run select_best_ml_tree_msa in parallel (one one single node) on all the MSAs """
  results_path = os.path.join(op.output_dir, "mlsearch_run", "results")
  pool = multiprocessing.Pool(processes=min(multiprocessing.cpu_count(), int(op.cores)))
  for name, msa in msas.items():
    if (not msa.valid):
      continue
    pool.apply_async(select_best_ml_tree_msa, (msa, results_path, op,))
  pool.close()
  pool.join()
