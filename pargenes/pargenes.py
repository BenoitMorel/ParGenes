#!/usr/bin/env python
import sys
import os
scriptdir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.join(scriptdir, 'pargenes_src'))
import pargenescore

args = sys.argv
args.append("--scheduler")
args.append("fork")
pargenescore.run_pargenes(args)


