import os
import logger
import sys
def makedirs(path):
  """ Create a directory if it does not exists yet """
  try:
    os.makedirs(path)
  except:
    pass

class MSA:
  """ group all the information related to one MSA """
  def __init__(self, name, path, raxml_arguments, modeltest_arguments):
    self.binary_path = ""
    self.taxa = 0
    self.per_taxon_clv_size = 0
    self.patterns = 0
    self.cores = 0
    self.model = ""
    self.raxml_args = []
    self.modeltest_arguments = ""
    self.flag_disable_sorting = False 

    self.name = name
    self.path = path
    self.valid = True
    self.add_raxml_arguments_str(raxml_arguments)
    self.modeltest_arguments = modeltest_arguments

  def set_model(self, model):
    self.model = model

  def get_model(self):
    return self.model

  def has_model(self):
    return self.get_model() != ""

  def add_raxml_arguments_str(self, arguments_str):
    self.add_raxml_arguments(arguments_str.split(" "))
  
  def add_raxml_arguments(self, arguments_list):
    last_was_model = False
    for arg in arguments_list:    
      if (last_was_model):
        self.set_model(arg)
        last_was_model = False
      elif (arg == "--model"):
        last_was_model = True
      else:
        self.raxml_args.append(arg)

  def get_raxml_arguments_str(self):
    return " ".join(self.raxml_args) + " --model " + self.get_model()
    

def get_msa_name(msa_file):
  return msa_file.replace(".", "_")

def add_per_msa_raxml_options(msas, options_file):
  """ Read the per-msa raxml options file, and append the options to
  their corresponding MSA """
  if (options_file == None):
    return
  per_msa_options = {}
  with open(options_file, "r") as reader:
    for line in reader.readlines():
      split = line[:-1].split(" ")
      msa = get_msa_name(split[0])
      if (not msa in msas):
        logger.warning("Found msa " + msa + " in options file " + options_file + " but not in the msas directory")
        continue
      msas[msa].add_raxml_arguments(split[1:])

def add_per_msa_modeltest_options(msas, options_file):
  """ Read the per-msa modeltest options file, and append the options to
  their corresponding MSA """
  if (options_file == None):
    return
  per_msa_options = {}
  with open(options_file, "r") as reader:
    for line in reader.readlines():
      split = line[:-1].split(" ")
      msa = get_msa_name(split[0])
      if (not msa in msas):
        logger.warning("Found msa " + msa + " in options file " + options_file + " but not in the msas directory")
        continue
      msas[msa].modeltest_arguments += " " + " ".join(split[1:])

def get_filter_content(msa_filter):
  """ Parse the (optional) file containing the list of MSAs to process"""
  if (msa_filter == None):
    return None
  msas_to_process = {}
  with open(msa_filter, "r") as reader:
    for line in reader.readlines():
      msa = line[:-1]
      msas_to_process[msa] = msa
  return msas_to_process

def exit_missing_model(msa):
  logger.error("You forgot to specify the substitution model for at least one msa (" + msa.name + ")")
  logger.error("To add it, you can either:")
  logger.error("- add \"--model model\" in the raxml options (see --per-msa-raxml-parameters or --raxml-global-parameters in ParGenes arguments)")
  logger.error("- let ModelTest find the best-fit model with -m")
  logger.error("ParGenes will now abort")
  sys.exit(30)

def init_msas(op):
  """ Init the list of MSAs from the user input arguments """
  msas = {}
  raxml_options = ""
  modeltest_options = ""
  if (op.raxml_global_parameters != None):
    raxml_options = open(op.raxml_global_parameters, "r").readlines()
    if (len(raxml_options) != 0):
      raxml_options = raxml_options[0][:-1]
    else:
      raxml_options = ""
  if (op.raxml_global_parameters_string != None):
    raxml_options += " " + op.raxml_global_parameters_string
  if (op.modeltest_global_parameters != None):
    modeltest_options = open(op.modeltest_global_parameters, "r").readlines()[0][:-1]
  msa_filter = get_filter_content(op.msa_filter)
  for f in os.listdir(op.alignments_dir):
    if (msa_filter != None):
      if (not (f in msa_filter)):
        continue
      del msa_filter[f]
    path = os.path.join(op.alignments_dir, f)
    name = get_msa_name(f)
    msas[name] = MSA(name, path, raxml_options, modeltest_options)
    if (op.disable_job_sorting):
      msas[name].flag_disable_sorting = True
    if (op.datatype == "aa"):
      msas[name].modeltest_arguments += " -d aa"
  if (msa_filter != None): # check that all files in the filter are present in the directory
    for msa in msa_filter.values():
      logger.warning("File " + msa + " was found in the filter file, but not in the MSAs directory")
  add_per_msa_raxml_options(msas, op.per_msa_raxml_parameters)
  add_per_msa_modeltest_options(msas, op.per_msa_modeltest_parameters)
  if (not op.use_modeltest):
    for msa in msas.values():
      if (not msa.has_model()):
        exit_missing_model(msa)
  return msas

def get_log_file(path, name, extension = "txt"):
  """ Build a log file. If it exists, use incremental naming to create
  a new one without erasing the previous one (useful when pargenes restarts)"""
  res = os.path.join(path, name + "." + extension)
  index = 1
  while (os.path.isfile(res)):
    res = os.path.join(path, name + "_"  + str(index) + "." + extension)
    index += 1 
  return res


