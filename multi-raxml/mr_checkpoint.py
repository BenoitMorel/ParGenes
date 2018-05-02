import os

def read_checkpoint(output_dir):
  """ Read and return the checkpoint value from the checkpoint file """
  try:
    f = open(os.path.join(output_dir, "checkpoint"))
    return int(f.readlines()[0])
  except:
    return 0

def write_checkpoint(output_dir, checkpoint):
  """ Write the current checkpoint value into the checkpoint file"""
  with open(os.path.join(output_dir, "checkpoint"), "w") as f:
    f.write(str(checkpoint))



