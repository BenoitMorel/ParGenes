#!/usr/bin/env python
import sys
import os
scriptdir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(scriptdir, 'pargenes_src'))
import pargenescore

import platform


args = sys.argv
args.append("--scheduler")
if ("darwin" in platform.system().lower()):
  print("Warning: ParGenes hpc mode is not supported on MacOS. ParGenes will run in hpc-debug mode instead. For more information, read the wiki page about running ParGenes on MacOS")
  args.append("onecore")
else:
  args.append("split")
pargenescore.run_pargenes(args)

