import pargenescore
import sys

args = sys.argv
args.append("--scheduler")
args.append("split")
pargenescore.run_pargenes(args)

