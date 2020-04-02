import os
import commons
import shutil
import time
import scheduler
import logger
import multiprocessing 

def run(output_dir, library, scheduler_mode, run_path, cores, op):
  """ Use the MPI scheduler to run raxml --support on all the MSAs. 
  This call builds the trees withs support values from bootstraps"""
  ml_trees_dir = os.path.join(output_dir, "mlsearch_run", "results")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  commands_file = os.path.join(run_path, "supports_commands.txt")
  commons.makedirs(run_path)
  support_results = os.path.join(run_path, "results")
  commons.makedirs(support_results)
  logger.info("Writing supports commands in " + commands_file)
  bs_metrics = ["", "tbe"]
  with open(commands_file, "w") as writer:
    for fasta in os.listdir(ml_trees_dir):
      for bs_metric in bs_metrics:
        ml_tree = os.path.join(ml_trees_dir, fasta, fasta + ".raxml.bestTree")
        bs_trees = os.path.join(concatenated_dir, fasta + ".bs")
        if (not os.path.exists(bs_trees) or os.stat(bs_trees).st_size == 0):
          continue
        writer.write("support_" + fasta)
        if (len(bs_metric) > 0):
          writer.write("_" + bs_metric)
        writer.write(" 1 1")
        writer.write(" --support")
        writer.write(" --tree " + ml_tree)
        writer.write(" --bs-trees " + bs_trees)
        if (len(bs_metric) > 0):
          writer.write(" --bs-metric " + bs_metric)
        if (scheduler_mode != "fork"):
          writer.write(" --threads 1")
        writer.write(" --prefix " + os.path.join(support_results, fasta + ".support"))
        if (len(bs_metric) > 0):
          writer.write("." + bs_metric)
        writer.write("\n") 
  scheduler.run_scheduler(library, scheduler_mode, "--threads", commands_file, run_path, cores, op)  


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
            logger.error("OS error when copying " + os.path.join(fasta_bs_dir, bs_file) + " to " + concatenated_file)
            raise e
  if (no_bs or os.stat(concatenated_file).st_size == 0):
    os.remove(concatenated_file)

def concatenate_bootstraps(output_dir, cores):
  """ Concurrently run concatenate_bootstrap_msa on all the MSA (on one single node)"""
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  commons.makedirs(concatenated_dir)
  bootstraps_dir = os.path.join(output_dir, "mlsearch_run", "bootstraps")
  pool = multiprocessing.Pool(processes=min(multiprocessing.cpu_count(), int(cores)))
  for msa_name in os.listdir(bootstraps_dir):
    pool.apply_async(concatenate_bootstrap_msa, (bootstraps_dir, concatenated_dir, msa_name,))
  pool.close()
  pool.join()


