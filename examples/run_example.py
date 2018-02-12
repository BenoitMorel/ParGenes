import os
import subprocess


def get_parent_path(path):
  return os.path.abspath(os.path.join(path, os.pardir))

root = get_parent_path(get_parent_path(os.path.realpath(__file__)))
binary = os.path.join(root, "build", "multi-raxml")
commandsFile = os.path.join(root, "examples", "commands", "command.txt")
threads = "4"

commandsStr = "mpirun -np 1 " + binary + " " + commandsFile + " " + threads
commands = commandsStr.split(" ")


print("Executing example " + commandsStr)
print("from " + root)

os.chdir(root)
subprocess.check_call(commands)




