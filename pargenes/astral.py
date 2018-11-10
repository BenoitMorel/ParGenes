import os
import sys
import subprocess

def get_gene_trees_file(pargenes_dir):
  return os.path.join(pargenes_dir, "astral_run", "gene_trees.newick")

def extract_gene_trees_ml(pargenes_dir, gene_trees_filename):
  #results = os.path.join(pargenes_dir, "supports_run", "results")
  print("not supported yet")
  exit(1)
  
def extract_gene_trees_ml(pargenes_dir, gene_trees_filename):
  results = os.path.join(pargenes_dir, "mlsearch_run", "results")
  count = 0
  with open(gene_trees_filename, "w") as writer:
    for msa in os.listdir(results):
      tree_file = os.path.join(results, msa, msa + ".raxml.bestTree")
      print(tree_file)
      try:
        with open(tree_file) as reader:
          writer.write(reader.read())
        count += 1
      except:
        continue
  print("Astral: " + str(count) + " trees were found in ParGenes output directory")
  
  
def extract_gene_trees(pargenes_dir, gene_trees_filename):
  """ Get all ParGenes ML gene trees and store then in gene_trees_filename 
  """
  with_supports = os.path.isdir(os.path.join(pargenes_dir, "supports_run"))
  results = ""
  if (with_supports):
    extract_gene_trees_support(pargenes_dir, gene_trees_filename)
  else:
    extract_gene_trees_ml(pargenes_dir, gene_trees_filename)

def run_astral(pargenes_dir, astral_args):
  """ Run astral on the ParGenes ML trees
  - pargenes_dir (str):        pargenes output directory
  - astral_args  (list(str)):  list of arguments to pass to astral
  """
  astral_jar = os.path.join("ASTRAL", "Astral", "astral.jar")
  astral_run_dir = os.path.join(pargenes_dir, "astral_run")
  astral_output = os.path.join(astral_run_dir, "output_species_tree.newick")
  astral_logs = os.path.join(astral_run_dir, "astral_logs.txt")
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
    command += astral_args + " "
  command = command[:-1]
  split_command = command.split(" ")
  out = open(astral_logs, "w")
  print("Starting astral... Logs will be redirected to " + astral_logs)
  p = subprocess.Popen(split_command, stdout=out, stderr=out)
  p.wait()
  if (0 != p.returncode):
    print("Error: Astral execution failed")
    exit(1)

if (__name__ == '__main__'):
  if (len(sys.argv) != 2):
    print("Error: syntax is:")
    print("python astral.py pargenes_dir")
    exit(1)
  pargenes_dir = sys.argv[1]
  run_astral(pargenes_dir, [])
  print("astral.py finished")

