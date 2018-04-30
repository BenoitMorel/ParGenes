import os
import mr_commons
import shutil
import time
import queue
import concurrent.futures
import mr_scheduler

def run(output_dir, library, scheduler, run_path, cores):
  ml_trees_dir = os.path.join(output_dir, "mlsearch_run", "results")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  commands_file = os.path.join(run_path, "supports_commands.txt")
  mr_commons.makedirs(run_path)
  print("Writing supports commands in " + commands_file)
  with open(commands_file, "w") as writer:
    for fasta in os.listdir(ml_trees_dir):
      ml_tree = os.path.join(ml_trees_dir, fasta, fasta + ".raxml.bestTree")
      bs_trees = os.path.join(concatenated_dir, fasta + ".bs")
      writer.write("support_" + fasta + " 1 1")
      writer.write(" --support")
      writer.write(" --tree " + ml_tree)
      writer.write(" --bs-trees " + bs_trees)
      writer.write(" --threads 1")
      writer.write(" --prefix " + os.path.join(run_path, fasta + ".support"))
      writer.write("\n") 
  mr_scheduler.run_mpi_scheduler(library, scheduler, commands_file, run_path, cores)  


def concatenate_bootstrap_msa(bootstraps_dir, concatenated_dir, msa_name):
  concatenated_file = os.path.join(concatenated_dir, msa_name + ".bs")
  with open(concatenated_file,'wb') as writer:
    fasta_bs_dir = os.path.join(bootstraps_dir, msa_name)
    for bs_file in os.listdir(fasta_bs_dir):
      if (bs_file.endswith("bootstraps")):
        with open(os.path.join(fasta_bs_dir, bs_file),'rb') as reader:
          try:
            shutil.copyfileobj(reader, writer)
          except OSError as e:
            print("ERROR!")
            print("OS error when copying " + os.path.join(fasta_bs_dir, bs_file) + " to " + concatenated_file)
            raise e

def concatenate_bootstraps(output_dir):
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  mr_commons.makedirs(concatenated_dir)
  bootstraps_dir = os.path.join(output_dir, "mlsearch_run", "bootstraps")
  with concurrent.futures.ThreadPoolExecutor() as e:
    for msa_name in os.listdir(bootstraps_dir):
      e.submit(concatenate_bootstrap_msa, bootstraps_dir, concatenated_dir, msa_name) 
    e.shutdown()




