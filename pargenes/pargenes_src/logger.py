import logging
import commons
import sys
import time
from datetime import timedelta

pargenes_logger = logging.getLogger("pargenes")
pargenes_logger.addHandler(logging.StreamHandler(sys.stdout))
pargenes_logger_start = time.time()



def init_logger(output_dir):
  global pargenes_logger
  logs = commons.get_log_file(output_dir, "pargenes_logs")
  pargenes_logger.addHandler(logging.FileHandler(logs))


def log(msg):
  global pargenes_logger
  pargenes_logger.error(msg)

def timed_log(msg, flag = ""):
  """ Print the time elapsed from initial_time and the msg """ 
  global pargenes_logger_start
  elapsed = time.time() - pargenes_logger_start
  log(flag + "[" + str(timedelta(seconds = int(elapsed))) + "] " + msg)

def info(msg):
  log(msg)


def warning(msg):
  global pargenes_logger_start
  timed_log(msg, "[Warning] " )

def error(msg):
  timed_log(msg, "[Error] ")

