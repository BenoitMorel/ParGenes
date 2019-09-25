import pargenescore
import sys
import platform


args = sys.argv
args.append("--scheduler")
if ("darwin" in platform.system().lower()):
  print("Warning: ParGenes hpc mode is not supported on MacOS. ParGenes will run in hpc-debug mode instead. For more information, read the wiki page about running ParGenes on MacOS")
  args.append("one-core")
else:
  args.append("split")
pargenescore.run_pargenes(args)

