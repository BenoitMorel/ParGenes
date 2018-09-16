import os
import sys
import subprocess
import shutil
import time
import shlex

tests_path = os.path.dirname(os.path.realpath(__file__)) 
root = os.path.dirname(tests_path)
pargenes_path = os.path.join(root, "pargenes", "pargenes.py")
example_data_path = os.path.join(tests_path, "smalldata")
example_msas = os.path.join(example_data_path, "fasta_files")
example_raxml_options = os.path.join(example_data_path, "raxml_global_options.txt")
tests_output_dir = os.path.join(tests_path, "tests_outputs")
example_modeltest_parameters = os.path.join(example_data_path, "only_1_models.txt")
schedulers = ["openmp", "split", "onecore"]
valid_msas = ["msa1_fasta", "msa2_fasta", "msa3_fasta", "msa4_fasta"]
best_models = ["JC", "F81+FU", "JC", "JC"]
invalid_msas = ["msa5_fasta"]
  

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

def check_all(run_dir, parse, modeltest, mlsearch):
  if (parse):
    check_parse(run_dir)
  if (modeltest):
    check_modeltest(run_dir)
  if (mlsearch):
    check_ml_search(run_dir)
  
def run_command(command, run_name):
  start_time = time.time()
  logs = os.path.join(tests_output_dir, run_name + ".txt")
  sys.stdout.write("[" + run_name + "]: ")
  sys.stdout.flush()
  with open(logs, "wb", 0) as out:
    subprocess.check_call(shlex.split(command), stdout = out)
  print("Success! (" + str(int((time.time() - start_time))) + "s)")

def test_help():
  command = "python " + pargenes_path + " -h"
  run_command(command, "help")

def test_ml_search(schedulers):
  output = os.path.join(tests_output_dir, "test_ml_search", schedulers)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = "python " + pargenes_path + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-r " + example_raxml_options + " "
  command += "-c 4 "
  command += "--scheduler " + scheduler
  run_command(command, "ml_search_" + schedulers )
  check_all(output, True, False, True)

def test_model_test(schedulers):
  output = os.path.join(tests_output_dir, "test_modeltest", schedulers)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = "python3 " + pargenes_path + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-c 4 "
  command += "--scheduler " + scheduler
  command += " -m"
  command += " --modeltest-global-parameters " + example_modeltest_parameters
  run_command(command, "modeltest_" + scheduler)
  check_all(output, True, True, True)

def test_bootstraps(schedulers):
  output = os.path.join(tests_output_dir, "test_bootstraps", schedulers)
  try:
    shutil.rmtree(output)
  except:
    pass
  command = "python3 " + pargenes_path + " "
  command += "-a " + example_msas + " "
  command += "-o " + output + " "
  command += "-r " + example_raxml_options + " "
  command += "-c 4 "
  command += "--scheduler " + scheduler
  command += " -b 3"
  run_command(command, "bootstraps" + schedulers )
  check_all(output, True, False, True)


try:
  os.makedirs(tests_output_dir)
except:
  pass

test_help()

for scheduler in schedulers:
  test_ml_search(scheduler)
  test_model_test(scheduler)
  test_bootstraps(scheduler)
