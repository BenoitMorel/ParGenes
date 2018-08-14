import os
import sys
import subprocess
import shutil
import time
import shlex

tests_path = os.path.dirname(os.path.realpath(__file__)) 
root = os.path.dirname(tests_path)
pargenes_path = os.path.join(root, "pargenes", "pargenes.py")
example_data_path = os.path.join(root, "examples", "data", "small")
example_msas = os.path.join(example_data_path, "fasta_files")
example_raxml_options = os.path.join(example_data_path, "raxml_global_options.txt")
tests_output_dir = os.path.join(tests_path, "tests_outputs")
example_modeltest_parameters = os.path.join(example_data_path, "only_1_models.txt")
schedulers = ["split", "openmp", "onecore"]

def run_command(command, run_name):
  start_time = time.time()
  logs = os.path.join(tests_output_dir, run_name + ".txt")
  print("[" + run_name + "]: ", end='')
  with open(logs, "wb", 0) as out:
    subprocess.check_call(shlex.split(command), stdout = out)
  print("Success! (" + str(int((time.time() - start_time))) + "s)")

def check_help():
  command = "python3 " + pargenes_path + " -h"
  run_command(command, "help")

def check_ml_search(schedulers):
  output = os.path.join(tests_output_dir, "test_ml_search", schedulers)
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
  run_command(command, "ml_search_" + schedulers )


def check_model_test(schedulers):
  output = os.path.join(tests_output_dir, "test_modeltest", schedulers)
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
  command += " -m"
  command += " --modeltest-global-parameters " + example_modeltest_parameters
  run_command(command, "modeltest_" + scheduler)

def check_bootstraps(schedulers):
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
  command += " -b 5"
  run_command(command, "bootstraps" + schedulers )


try:
  os.makedirs(tests_output_dir)
except:
  pass

check_help()

for scheduler in schedulers:
  check_ml_search(scheduler)
  check_model_test(scheduler)
  check_bootstraps(scheduler)
