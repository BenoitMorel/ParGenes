import os
import sys
import subprocess
import arguments
import logger
import time

def treat_newick(newick):
  return newick.replace("@", "at")

def get_gene_trees_file(pargenes_dir):
  return os.path.join(pargenes_dir, "astral_run", "gene_trees.newick")

def extract_gene_trees_support(pargenes_dir, gene_trees_filename):
  results = os.path.join(pargenes_dir, "supports_run", "results")
  count = 0
  with open(gene_trees_filename, "w") as writer:
    for f in os.listdir(results):
      if (f.ends_with(".raxml.support")):
        with open(f) as reader:
          writer.write(treat_newick(reader.read()))
        count += 1
  logger.info("ParGenes/Astral: " + str(count) + " gene trees (with support values) were found in ParGenes output directory")
  
def extract_gene_trees_ml(pargenes_dir, gene_trees_filename):
  results = os.path.join(pargenes_dir, "mlsearch_run", "results")
  count = 0
  with open(gene_trees_filename, "w") as writer:
    for msa in os.listdir(results):
      tree_file = os.path.join(results, msa, msa + ".raxml.bestTree")
      try:
        with open(tree_file) as reader:
          writer.write(treat_newick(reader.read()))
        count += 1
      except:
        continue
  logger.info("ParGenes/Astral: " + str(count) + " trees were found in ParGenes output directory")
  
  
def extract_gene_trees(pargenes_dir, gene_trees_filename):
  """ Get all ParGenes ML gene trees and store then in gene_trees_filename 
  """
  with_supports = os.path.isdir(os.path.join(pargenes_dir, "supports_run"))
  results = ""
  if (with_supports):
    extract_gene_trees_support(pargenes_dir, gene_trees_filename)
  else:
    extract_gene_trees_ml(pargenes_dir, gene_trees_filename)

def get_additional_parameters(parameters_file):
  if (parameters_file == None or parameters_file == ""):
    return []
  astral_options = open(parameters_file, "r").readlines()
  if (len(astral_options) != 0):
    return astral_options[0][:-1].split(" ")
  else:
    return [] 
  
  
def run_astral(pargenes_dir, astral_jar, parameters_file):
  """ Run astral on the ParGenes ML trees
  - pargenes_dir     (str):        pargenes output directory
  - astral_jar       (str):        path to astral jar
  - parameters_file  (list(str)):  a file with the list of additional arguments to pass to astral
  """
  astral_args = get_additional_parameters(parameters_file)
  astral_run_dir = os.path.join(pargenes_dir, "astral_run")
  astral_output = os.path.join(astral_run_dir, "output_species_tree.newick")
  astral_logs = os.path.join(astral_run_dir, "astral_logs.txt")
  logger.info("Astral additional parameters:")
  logger.info(astral_args)
  try:
    os.makedirs(astral_run_dir)
  except:
    pass
  gene_trees = get_gene_trees_file(pargenes_dir)
  extract_gene_trees(pargenes_dir, gene_trees)
  command = ""
  command += "java -jar "
  command += astral_jar + " "
  command += "-i " + gene_trees + " "
  command += "-o " + astral_output + " " 
  for arg in astral_args:
    command += arg + " "
  command = command[:-1]
  split_command = command.split(" ")
  out = open(astral_logs, "w")
  logger.info("Starting astral... Logs will be redirected to " + astral_logs)
  p = subprocess.Popen(split_command, stdout=out, stderr=out)
  p.wait()
  if (0 != p.returncode):
    logger.error("Astral execution failed")
    exit(1)


def run_astral_pargenes(astral_jar, op):
  pargenes_dir = op.output_dir
  scriptdir = os.path.dirname(os.path.realpath(__file__))
  astral_jar = os.path.join(scriptdir, "..", "ASTRAL", "Astral", "astral.jar")
  run_astral(pargenes_dir, astral_jar, op.astral_global_parameters)

if (__name__ == '__main__'):
  if (len(sys.argv) != 2 and len(sys.argv) != 3):
    print("Error: syntax is:")
    print("python astral.py pargenes_dir [astral_parameters_file]")
    print("astral_parameters_file is optional and should contain additional parameters to pass to astral call)")
    exit(1)
  pargenes_dir = sys.argv[1]
  parameters_file = None
  if (len(sys.argv) > 2):
    parameters_file = sys.argv[2]
  start = time.time()
  logger.init_logger(pargenes_dir)
  logger.timed_log(start, "Starting astral pargenes script...")
  scriptdir = os.path.dirname(os.path.realpath(__file__))
  astral_jar = os.path.join(scriptdir, "..", "ASTRAL", "Astral", "astral.jar")
  run_astral(pargenes_dir, astral_jar, parameters_file)
  logger.timed_log(start, "End of astral pargenes script...")

