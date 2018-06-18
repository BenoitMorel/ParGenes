import os
import mr_commons
import shutil
import time
import queue
import concurrent.futures
import mr_scheduler

def run(output_dir, library, scheduler, run_path, cores):
  """ Use the MPI scheduler to run raxml --support on all the MSAs. 
  This call builds the trees withs support values from bootstraps"""
  ml_trees_dir = os.path.join(output_dir, "mlsearch_run", "results")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  commands_file = os.path.join(run_path, "supports_commands.txt")
  mr_commons.makedirs(run_path)
  support_results = os.path.join(run_path, "results")
  mr_commons.makedirs(support_results)
  print("Writing supports commands in " + commands_file)
  with open(commands_file, "w") as writer:
    for fasta in os.listdir(ml_trees_dir):
      ml_tree = os.path.join(ml_trees_dir, fasta, fasta + ".raxml.bestTree")
      bs_trees = os.path.join(concatenated_dir, fasta + ".bs")
      if (not os.path.exists(bs_trees) or os.stat(bs_trees).st_size == 0):
        continue
      writer.write("support_" + fasta + " 1 1")
      writer.write(" --support")
      writer.write(" --tree " + ml_tree)
      writer.write(" --bs-trees " + bs_trees)
      writer.write(" --threads 1")
      writer.write(" --prefix " + os.path.join(support_results, fasta + ".support"))
      writer.write("\n") 
  mr_scheduler.run_mpi_scheduler(library, scheduler, commands_file, run_path, cores)  


def concatenate_bootstrap_msa(bootstraps_dir, concatenated_dir, msa_name):
  """ For a given MSA, concatenates the bootstraps trees
  from the independent raxml runs"""
  concatenated_file = os.path.join(concatenated_dir, msa_name + ".bs")
  no_bs = True
  with open(concatenated_file,'wb') as writer:
    fasta_bs_dir = os.path.join(bootstraps_dir, msa_name)
    for bs_file in os.listdir(fasta_bs_dir):
      if (bs_file.endswith("bootstraps")):
        with open(os.path.join(fasta_bs_dir, bs_file),'rb') as reader:
          try:
            no_bs = False
            shutil.copyfileobj(reader, writer)
          except OSError as e:
            print("ERROR!")
            print("OS error when copying " + os.path.join(fasta_bs_dir, bs_file) + " to " + concatenated_file)
            raise e
  if (no_bs or os.stat(concatenated_file).st_size == 0):
    os.remove(concatenated_file)

def concatenate_bootstraps(output_dir, cores):
  """ Concurrently run concatenate_bootstrap_msa on all the MSA (on one single node)"""
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  mr_commons.makedirs(concatenated_dir)
  bootstraps_dir = os.path.join(output_dir, "mlsearch_run", "bootstraps")
  with concurrent.futures.ThreadPoolExecutor(max_workers = int(cores)) as e:
    for msa_name in os.listdir(bootstraps_dir):
      e.submit(concatenate_bootstrap_msa, bootstraps_dir, concatenated_dir, msa_name) 



