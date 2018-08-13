import os
import sys
import subprocess
import shutil

tests_path = os.path.dirname(os.path.realpath(__file__)) 
root = os.path.dirname(tests_path)
pargenes_path = os.path.join(root, "pargenes", "pargenes.py")
example_data_path = os.path.join(root, "examples", "data", "small")
example_msas = os.path.join(example_data_path, "fasta_files")
example_raxml_options = os.path.join(example_data_path, "raxml_global_options.txt")
tests_output_dir = os.path.join(tests_path, "tests_outputs")

schedulers = ["split", "openmp", "onecore"]

def check_help():
  command = "python3 " + pargenes_path + " -h"
  subprocess.check_call(command.split(" "))

def check_ml_search(schedulers):
  output = os.path.join(tests_output_dir, "test_ml_search")
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
  subprocess.check_call(command.split(" "))


check_help()

for scheduler in schedulers:
  check_ml_search(scheduler)

