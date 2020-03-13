#!/usr/bin/env python
import pargenescore
import sys

args = sys.argv
args.append("--scheduler")
args.append("onecore")
pargenescore.run_pargenes(args)


