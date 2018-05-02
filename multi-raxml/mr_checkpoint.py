import os

def read_checkpoint(output_dir):
  try:
    f = open(os.path.join(output_dir, "checkpoint"))
    return int(f.readlines()[0])
  except:
    return 0

def write_checkpoint(output_dir, checkpoint):
  with open(os.path.join(output_dir, "checkpoint"), "w") as f:
    f.write(str(checkpoint))



