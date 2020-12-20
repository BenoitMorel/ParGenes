import os
import sys
import argparse
import shutil

def get_input_parameters():
  parser = argparse.ArgumentParser(sys.argv)
  
  parser.add_argument('-o', "--output", 
      dest="output_dir",
      required = True,
      help="Output directory (will be created)")
  
  parser.add_argument('-i', "--input", 
      dest="input_dir",
      required = True,
      help="Output directory (will be created)")

  parser.add_argument("--best-ml-tree",
      dest="best_ml_tree",
      action="store_true",
      default=False,
      help="For each family, export the best ML tree")
  
  parser.add_argument("--best-ml-model",
      dest="best_ml_model",
      action="store_true",
      default=False,
      help="For each family, export the substitution model for the best ML tree")
  
  parser.add_argument("--bootstrap-trees",
      dest="bootstrap_trees",
      action="store_true",
      default=False,
      help="For each family, export the bootstrap trees")
  
  parser.add_argument("--support-values-tree",
      dest="support_value_tree",
      action="store_true",
      default=False,
      help="For each family, export the best ML tree with support values")
  
  return parser.parse_args()

def check_parameters(p):
  if (os.path.exists(p.output_dir)):
    print("Error: the output directory already exists.")
    sys.exit(1)
  if (not os.path.isdir(p.input_dir)):
    print("Error: the input pargenes directory does not exist.")
    sys.exit(1)
  logs_file = os.path.join(p.input_dir, "pargenes_logs.txt")
  if (not os.path.isfile(logs_file)):
    print("Error, could not find the file " + logs_file)
    print("The input directory might not be a pargenes directory")
    print("If the input directory IS a pargenes directory but you removed the logs file, just create it again (empty) to bypass this check")
    sys.exit(1)

def get_families(input_dir):
  parse_results_dir = os.path.join(input_dir, "mlsearch_run", "results")
  return os.listdir(parse_results_dir)

def export_best_ml_tree(input_dir, output_dir, families):
  print("--> For each family, exporting best ML tree...")
  results_dir = os.path.join(input_dir, "mlsearch_run", "results")
  for family in families:
    tocopy = os.path.join(results_dir, family, family + ".raxml.bestTree")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_best_ml_model(input_dir, output_dir, families):
  print("--> For each family, exporting substitution model for the best ML run...")
  results_dir = os.path.join(input_dir, "mlsearch_run", "results")
  for family in families:
    tocopy = os.path.join(results_dir, family, family + ".raxml.bestModel")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_bootstrap_trees(input_dir, output_dir, families):
  print("--> For each family, exporting bootstrap trees...")
  results_dir = os.path.join(input_dir, "concatenated_bootstraps")
  for family in families:
    tocopy = os.path.join(results_dir, family + ".bs")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_support_value_trees(input_dir, output_dir, families):
  print("--> For each family, exporting best ML tree with support values")
  results_dir = os.path.join(input_dir, "supports_run", "results")
  for family in families:
    tocopy = os.path.join(results_dir, family + ".support.raxml.support")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

if (__name__== "__main__"):
  p = get_input_parameters()
  print("")
  print("Start exporting files from pargenes run located in: " + p.output_dir)
  print("into the output directory: " + p.input_dir)
  print("")
  check_parameters(p)
  os.makedirs(p.output_dir)
  families = get_families(p.input_dir)
  if (p.best_ml_tree):
    export_best_ml_tree(p.input_dir, p.output_dir, families)
  if (p.best_ml_model):
    export_best_ml_model(p.input_dir, p.output_dir, families)
  if (p.bootstrap_trees):
    export_bootstrap_trees(p.input_dir, p.output_dir, families)
  if (p.support_value_tree):
    export_support_value_trees(p.input_dir, p.output_dir, families)
  print("")
  print("Export successfully finished. Results in " + p.output_dir)


