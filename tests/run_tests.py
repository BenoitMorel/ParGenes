import os
import sys
import subprocess
import shutil
import time
import shlex
import platform

run_as_binary = "--run-as-binary" in sys.argv

tests_path = os.path.dirname(os.path.realpath(__file__))
root = os.path.dirname(tests_path)
pargenes_script_names = ["pargenes.py", "pargenes-hpc.py", "pargenes-hpc-debug.py"]
pargenes_scripts = []
if (run_as_binary):
  pargenes_scripts = pargenes_script_names
else:
  for s in pargenes_script_names:
    pargenes_scripts.append("python " + os.path.join(root, "pargenes", s))
example_data_path = os.path.join(tests_path, "smalldata")
example_msas = os.path.join(example_data_path, "fasta_files")
example_raxml_options = os.path.join(example_data_path, "raxml_global_options.txt")
tests_output_dir = os.path.join(tests_path, "tests_outputs")
example_modeltest_parameters = os.path.join(example_data_path, "only_1_models.txt")
valid_msas = ["msa1_fasta", "msa2_fasta", "msa3_fasta", "msa4_fasta"]
best_models = ["JC", "F81+FU", "JC", "JC"]
invalid_msas = ["msa5_fasta"]

def get_basename(plop):
  return os.path.basename(plop).split(".")[0]

def check_parse(run_dir):
  invalid_msas_file = os.path.join(run_dir, "invalid_msas.txt")
  assert os.path.isfile(invalid_msas_file)
  lines = open(invalid_msas_file).readlines()
  assert len(lines) == 1
  assert lines[0][:-1] == invalid_msas[0]
  for msa in valid_msas:
    results = os.path.join(run_dir, "parse_run", "results", msa)
    rba = os.path.join(results, msa + ".raxml.rba")
    log = os.path.join(results, msa + ".raxml.log")
    assert os.path.isdir(results)
    assert os.path.isfile(rba)
    assert os.path.isfile(log)

def check_modeltest(run_dir):
  modeltest_dir = os.path.join(run_dir, "modeltest_run")
  assert len(os.listdir(os.path.join(modeltest_dir, "running_jobs"))) == 0
  mlsearch_dir = os.path.join(run_dir, "mlsearch_run")
  assert len(os.listdir(os.path.join(mlsearch_dir, "running_jobs"))) == 0
  for msa, best_model in list(zip(valid_msas, best_models)):
    results = os.path.join(modeltest_dir, "results", msa)
    assert os.path.isdir(results)
    assert os.path.isfile(os.path.join(results, msa + ".log"))
    results = os.path.join(mlsearch_dir, "results", msa)
    bestModel = os.path.join(results, msa + ".raxml.bestModel")
    assert os.path.isfile(bestModel)
    model = open(bestModel).readlines()[0].split(",")[0].split("{")[0]
    assert model == best_model

def check_ml_search(run_dir):
  mlsearch_dir = os.path.join(run_dir, "mlsearch_run")
  assert len(os.listdir(os.path.join(mlsearch_dir, "running_jobs"))) == 0
  for msa in valid_msas:
    results = os.path.join(mlsearch_dir, "results", msa)
    prefix = os.path.join(results, msa + ".raxml.")
    assert os.path.isdir(results)
    assert os.path.isfile(prefix + "bestTree")
    assert os.path.isfile(prefix + "bestModel")
    assert os.path.isfile(prefix + "log")

def check_astral(run_dir):
  astral_dir = os.path.join(run_dir, "astral_run")
  input_gene_trees = os.path.join(astral_dir, "gene_trees.newick")
  output_species_tree = os.path.join(astral_dir, "output_species_tree.newick")
  assert(os.path.isfile(input_gene_trees))
  assert(os.path.isfile(output_species_tree))
  input_gene_trees_lines = open(input_gene_trees).readlines()
  assert(len(input_gene_trees_lines) == 4)
  output_species_tree_str = open(output_species_tree).readlines()[0]
  for i in range(0, 10):
    assert(("taxa_" + str(i)) in output_species_tree_str)

def check_all(run_dir, parse, modeltest, mlsearch, astral):
  try:
    if (parse):
      check_parse(run_dir)
    if (modeltest):
      check_modeltest(run_dir)
    if (mlsearch):
      check_ml_search(run_dir)
    if (astral):
      check_astral(run_dir)
  except:
    print("FAILURE!!!!")
    return 1
  print("Success!")
  return 0

def run_command(command, run_name, outputdir):
  #print(command)
  logs = os.path.join(tests_output_dir, run_name + ".txt")
  sys.stdout.write("[" + run_name + "]: ")
  sys.stdout.flush()
  with open(logs, "wb", 0) as out:
    try:
      subprocess.check_call(shlex.split(command), stdout = out)
    except:
      if (outputdir != None):
        if ("--report" in sys.argv):
          report_file = os.path.join(outputdir, "report.txt")
          if (os.path.isfile(report_file)):
            print("Printing the report file content: ")
            print(open(report_file).read())
          else:
            print("No report file found in " + report_file)
        else:
          print("Not trying to print report file")
      print("Test failed, restarting with logs enabled and continue option")
      subprocess.check_call(shlex.split(command + " --continue"))

def test_help():
  command = pargenes_scripts[0] + " -h"
  run_command(command, "help", outputdir = None)

def test_ml_search(pargenes_script):
  basename = get_basename(pargenes_script)
  output = os.path.join(tests_output_dir, "test_ml_search", basename)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = pargenes_script + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-r " + example_raxml_options + " "
  command += "-c 4 "
  command += "-s 3 -p 3 "
  run_command(command, "ml_search_" + basename, output)
  return check_all(output, True, False, True, False)

def test_model_test(pargenes_script):
  basename = get_basename(pargenes_script)
  output = os.path.join(tests_output_dir, "test_modeltest", basename)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = pargenes_script + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-c 4 "
  command += " -m"
  command += " --modeltest-global-parameters " + example_modeltest_parameters
  run_command(command, "modeltest_" + basename, output)
  return check_all(output, True, True, True, False)

def test_bootstraps(pargenes_script):
  basename = get_basename(pargenes_script)
  output = os.path.join(tests_output_dir, "test_bootstraps", basename)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = pargenes_script + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-r " + example_raxml_options + " "
  command += "-c 4 "
  command += " -b 3"
  run_command(command, "bootstraps" + basename, output)
  return check_all(output, True, False, True, False)

def test_astral(pargenes_script):
  basename = get_basename(pargenes_script)
  output = os.path.join(tests_output_dir, "test_astral", basename)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = pargenes_script + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-r " + example_raxml_options + " "
  command += "-c 4 "
  command += "-s 3 -p 3 "
  command += "--use-astral "
  run_command(command, "astral_" + basename, output)
  return check_all(output, True, False, False, True)

def test_all(pargenes_script):
  basename = get_basename(pargenes_script)
  output = os.path.join(tests_output_dir, "test_all", basename)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = pargenes_script + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-r " + example_raxml_options + " "
  command += "-c 4 "
  command += "-m "
  command += "-b 3 "
  command += "-s 3 -p 3 "
  command += "--use-astral "
  command += " --modeltest-global-parameters " + example_modeltest_parameters
  run_command(command, "all_" + basename, output)
  return  check_all(output, True, True, True, True)

try:
  os.makedirs(tests_output_dir)
except:
  pass

test_help()
failures = 0
for pargenes_script in pargenes_scripts:
  failures += test_ml_search(pargenes_script)
  failures += test_model_test(pargenes_script)
  failures += test_bootstraps(pargenes_script)
  failures += test_astral(pargenes_script)
  failures += test_all(pargenes_script)

print("")
if (failures > 0):
  print("ERROR: " + str(failures) + " tests failed. Please check you installation.")
  sys.exit(1)
else:
  print("All tests ran successfully")

