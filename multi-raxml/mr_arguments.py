import argparse

# parse the command line and return the arguments
def parse_arguments():
  parser = argparse.ArgumentParser()
  # general arguments
  parser.add_argument('-a', "--alignments-dir", 
      dest="alignments_dir", 
      help="Directory containing the fasta files")
  parser.add_argument('-o', "--output-dir", 
      dest="output_dir", 
      help="Output directory")
  parser.add_argument("-c", "--cores", 
      dest="cores",
      type=int,
      help="The number of computational cores available for computation")
  parser.add_argument("--continue",
      dest="do_continue",
      action="store_true",
      default=False,
      help="Allow multi-raxml to continue the analysis from the last checkpoint")
  parser.add_argument("--msa-filter",
      dest="msa_filter", 
      help="A file containing the names of the msa files to process")
  parser.add_argument("-d", "--datatype",
      dest="datatype",
      choices=["nt", "aa"],
      default="nt",
      help="Alignments datatype")
  # raxml arguments
  parser.add_argument("--per-msa-raxml-parameters", 
      dest="per_msa_raxml_parameters", 
      help="A file containing per-msa raxml parameters")
  parser.add_argument("-s", "--raxml-starting-trees", 
      dest="starting_trees", 
      type=int,
      default=1,
      help="The number of starting trees")
  parser.add_argument("-r", "--raxml-global-parameters", 
      dest="raxml_global_parameters", 
      help="A file containing the parameters to pass to raxml")
  parser.add_argument("-R", "--raxml-global-parameters-string", 
      dest="raxml_global_parameters_string", 
      help="List of parameters to pass to raxml (see also --raxml-global-parameters)")
  parser.add_argument("-b", "--bs-trees", 
      dest="bootstraps", 
      type=int,
      default=0,
      help="The number of bootstrap trees to compute")
  # modeltest arguments
  parser.add_argument("-m", "--use-modeltest",
      dest="use_modeltest",
      action="store_true",
      default=False,
      help="Autodetect the model with modeltest")
  parser.add_argument("--modeltest-global-parameters", 
      dest="modeltest_global_parameters", 
      help="A file containing the parameters to pass to modeltest")
  parser.add_argument("--per-msa-modeltest-parameters",
      dest="per_msa_modeltest_parameters", 
      help="A file containing per-msa modeltest parameters")
  parser.add_argument("--modeltest-criteria",
      dest="modeltest_criteria",
      choices=["AICc", "AIC", "BIC"],
      default="AICc",
      help="Alignments datatype")

  return parser.parse_args()

