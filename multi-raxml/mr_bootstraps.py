import os
import mr_commons
import shutil
import time
import queue
import concurrent.futures
def build_supports_commands(output_dir):
  ml_trees_dir = os.path.join(output_dir, "mlsearch_run", "results")
  concatenated_dir = os.path.join(output_dir, "concatenated_bootstraps")
  supports_commands_file = os.path.join(output_dir, "supports_commands.txt")
  support_dir = os.path.join(output_dir, "supports_run")
  mr_commons.makedirs(support_dir)
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




