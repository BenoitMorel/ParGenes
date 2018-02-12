import sys
import subprocess
import os


commands = sys.argv[2:]
FNULL = open(os.devnull, 'w')
subprocess.check_call(commands, stdout=FNULL, stderr=FNULL)
f = open(sys.argv[1], "w")

