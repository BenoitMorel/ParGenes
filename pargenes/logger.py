import logging
import commons
import sys
import time
from datetime import timedelta

pargenes_logger = logging.getLogger("pargenes")
pargenes_logger.addHandler(logging.StreamHandler(sys.stdout))

def init_logger(op):
  global pargenes_logger
  print("init logger")
  logs = commons.get_log_file(op.output_dir, "pargenes_logs")
  pargenes_logger.addHandler(logging.FileHandler(logs))

def info(msg):
  global pargenes_logger
  pargenes_logger.error(msg)

def warning(msg):
  info("[Warning] " + msg)

def error(msg):
  info("[Error] " + msg)


def timed_log(initial_time, msg):
  """ Print the time elapsed from initial_time and the msg """ 
  elapsed = time.time() - initial_time
  info("[" + str(timedelta(seconds = int(elapsed))) + "] " + msg)

