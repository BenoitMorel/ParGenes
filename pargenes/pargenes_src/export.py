import os
import sys
import argparse
import shutil

"""
Export relevant files from a run with ParGenes
"""

def get_input_parameters():
  parser = argparse.ArgumentParser()
  parser.add_argument("-i", "--input",
    dest = "input_dir",
    required = True,
    help = "Input directory (ParGenes output)")
  parser.add_argument("-o", "--output",
    dest = "output_dir",
    required = True,
    help = "Output directory (will be created)")
  parser.add_argument("--append",
    dest = "append",
    action = "store_true",
    default = False,
    help = "Append to Output directory")
  parser.add_argument("--best-ml-tree",
    dest = "best_ml_tree",
    action = "store_true",
    default = False,
    help = "For each family, export the best ML tree")
  parser.add_argument("--best-ml-model",
    dest = "best_ml_model",
    action = "store_true",
    default = False,
    help = "For each family, export the substitution model for the best ML tree")
  parser.add_argument("--bootstrap-trees",
    dest = "bootstrap_trees",
    action = "store_true",
    default = False,
    help = "For each family, export the bootstrap trees")
  parser.add_argument("--support-values-tree",
    dest = "support_value_tree",
    action = "store_true",
    default = False,
    help = "For each family, export the best ML tree with support values")
  parser.add_argument("--species-tree",
    dest = "species_tree",
    action = "store_true",
    default = False,
    help = "Export the species tree from ASTRAL/ASTER")
  parser.add_argument("--gene-trees",
    dest = "gene_trees",
    action = "store_true",
    default = False,
    help = "Export the gene-trees file used as input to ASTRAL/ASTER")
  parser.add_argument("--export-all", "-a",
    dest = "export_all",
    action = "store_true",
    default = False,
    help = "Export using all options (if applicable) sorted in to separate directories")
  return parser.parse_args()

def check_parameters(parms):
  if os.path.exists(p.output_dir):
    if parms.append:
      print(f"Adding output to existing directory {p.output_dir}")
    else:
      print(f"Error: the output directory {p.output_dir} already exists. Use --append to add to it.")
      sys.exit(1)
  if not os.path.isdir(parms.input_dir):
    print(f"Error: the input pargenes directory {p.input_dir} does not exist.")
    sys.exit(1)
  logs_file = os.path.join(parms.input_dir, "pargenes_logs.txt")
  if not os.path.isfile(logs_file):
    print("Error, could not find the file " + logs_file)
    print("The input directory might not be a pargenes directory")
    print("If the input directory IS a pargenes directory but you removed the logs file, just create it again (empty) to bypass this check")
    sys.exit(1)
  if all(not i for i in [parms.best_ml_tree, parms.best_ml_model, parms.bootstrap_trees, parms.support_value_tree, parms.species_tree, parms.gene_trees, parms.export_all]):
    print("Error: None of the export options are called. See -h for alternatives.")
    sys.exit(1)

def get_families(input_dir):
  parse_results_dir = os.path.join(input_dir, "mlsearch_run", "results")
  return os.listdir(parse_results_dir)

def export_best_ml_tree(input_dir, output_dir, fams):
  print("--> For each family, exporting best ML tree...")
  results_dir = os.path.join(input_dir, "mlsearch_run", "results")
  for fam in fams:
    tocopy = os.path.join(results_dir, fam, fam + ".raxml.bestTree")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_best_ml_model(input_dir, output_dir, fams):
  print("--> For each family, exporting substitution model for the best ML run...")
  results_dir = os.path.join(input_dir, "mlsearch_run", "results")
  for fam in fams:
    tocopy = os.path.join(results_dir, fam, fam + ".raxml.bestModel")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_bootstrap_trees(input_dir, output_dir, fams):
  print("--> For each family, exporting bootstrap trees...")
  results_dir = os.path.join(input_dir, "concatenated_bootstraps")
  if not os.path.isdir(results_dir):
    print(f"Error: the folder {results_dir} does not exist.")
    print("Was ParGenes run with boostraping (-b)?")
    sys.exit(1)
  for fam in fams:
    tocopy = os.path.join(results_dir, fam + ".bs")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_support_value_trees(input_dir, output_dir, fams):
  print("--> For each family, exporting best ML tree with support values...")
  results_dir = os.path.join(input_dir, "supports_run", "results")
  for fam in fams:
    tocopy = os.path.join(results_dir, fam + ".support.raxml.support")
    try:
      shutil.copy(tocopy, output_dir)
    except:
      print("Could not find " + tocopy)

def export_species_tree(input_dir, output_dir):
  print("--> Exporting species tree...")
  results_dir = os.path.join(input_dir, "astral_run")
  if not os.path.isdir(results_dir):
    results_dir = os.path.join(input_dir, "aster_run")
  if not os.path.isdir(results_dir):
    print("Error: the astral_run/aster_run directory does not exist.")
    print("Was ParGenes run with --use-astral or --use-aster?")
    sys.exit(1)
  tocopy = os.path.join(results_dir, "output_species_tree.newick")
  try:
    shutil.copy(tocopy, output_dir)
  except:
    print("Could not find " + tocopy)

def export_gene_trees(input_dir, output_dir):
  print("--> Exporting gene trees...")
  results_dir = os.path.join(input_dir, "astral_run")
  if not os.path.isdir(results_dir):
    results_dir = os.path.join(input_dir, "aster_run")
  if not os.path.isdir(results_dir):
    print("Error: the astral_run/aster_run directory does not exist.")
    print("Was ParGenes run with --use-astral or --use-aster?")
    sys.exit(1)
  tocopy = os.path.join(results_dir, "gene_trees.newick")
  try:
    shutil.copy(tocopy, output_dir)
  except:
    print("Could not find " + tocopy)

def export_all_output(input_dir, output_dir, fams):
  print("--> Exporting all ParGenes output...")
  if os.path.exists(os.path.join(p.input_dir, 'mlsearch_run')):
    out_dir = os.path.join(p.output_dir, "best_ml_tree")
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    export_best_ml_tree(p.input_dir, out_dir, fams)
    out_dir = os.path.join(p.output_dir, "best_ml_model")
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    export_best_ml_model(p.input_dir, out_dir, fams)
  if os.path.exists(os.path.join(p.input_dir, 'concatenated_bootstraps')):
    out_dir = os.path.join(p.output_dir, "bootstrap_trees")
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    export_bootstrap_trees(p.input_dir, out_dir, fams)
  if os.path.exists(os.path.join(p.input_dir, 'supports_run')):
    out_dir = os.path.join(p.output_dir, "support_value_trees")
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    export_support_value_trees(p.input_dir, out_dir, fams)
  if os.path.exists(os.path.join(p.input_dir, 'astral_run')) or os.path.exists(os.path.join(p.input_dir, 'aster_run')):
    out_dir = os.path.join(p.output_dir, "species_tree")
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    export_species_tree(p.input_dir, out_dir)
    out_dir = os.path.join(p.output_dir, "gene_trees")
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    export_gene_trees(p.input_dir, out_dir)

if __name__== "__main__":
  p = get_input_parameters()
  check_parameters(p)
  print("")
  print("Start exporting files from pargenes run located in: " + p.input_dir)
  print("into the output directory: " + p.output_dir)
  print("")
  if p.append:
    os.makedirs(p.output_dir, exist_ok=True)
  else:
    os.makedirs(p.output_dir)
  families = get_families(p.input_dir)
  if p.best_ml_tree:
    export_best_ml_tree(p.input_dir, p.output_dir, families)
  if p.best_ml_model:
    export_best_ml_model(p.input_dir, p.output_dir, families)
  if p.bootstrap_trees:
    export_bootstrap_trees(p.input_dir, p.output_dir, families)
  if p.support_value_tree:
    export_support_value_trees(p.input_dir, p.output_dir, families)
  if p.species_tree:
    export_species_tree(p.input_dir, p.output_dir)
  if p.gene_trees:
    export_gene_trees(p.input_dir, p.output_dir)
  if p.export_all:
    export_all_output(p.input_dir, p.output_dir, families)
  print("")
  print("Export successfully finished. Results in " + p.output_dir)
