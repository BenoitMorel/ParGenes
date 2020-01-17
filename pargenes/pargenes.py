import pargenescore
import sys

args = sys.argv
args.append("--scheduler")
args.append("fork")
pargenescore.run_pargenes(args)


