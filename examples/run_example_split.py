import os
import subprocess
import shutil

def get_parent_path(path):
  return os.path.abspath(os.path.join(path, os.pardir))

root = get_parent_path(get_parent_path(os.path.realpath(__file__)))
binary = os.path.join(root, "build", "multi-raxml")
commandsFile = os.path.join(root, "examples", "commands", "command.txt")
outputDir = os.path.join(root, "examples", "results")
threads = "4"

if os.path.exists(outputDir):
  shutil.rmtree(outputDir)  
os.makedirs(outputDir)

commandsStr = "mpirun -np " + threads + " "  + binary + " --split-scheduler " + commandsFile + " " + outputDir + " " + threads
commands = commandsStr.split(" ")


print("Executing example " + commandsStr)
print("from " + root)

os.chdir(root)
subprocess.check_call(commands)




