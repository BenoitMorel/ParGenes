import os
import mr_commons
import mr_scheduler

def run(msas, output_dir, library, run_path, op): 
  """ Use the MPI scheduler to run modeltest on all the MSAs"""
  scheduler = op.scheduler
  cores = op.cores
  run_path = os.path.join(output_dir, "modeltest_run")
  commands_file = os.path.join(run_path, "modeltest_command.txt")
  modeltest_results = os.path.join(run_path, "results")
  mr_commons.makedirs(modeltest_results)
  if (op.modeltest_cores < 4):
    print("[Error] The number of cores per modeltest job should at least be 4")
    sys.exit(1)
  with open(commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      modeltest_fasta_output_dir = os.path.join(modeltest_results, name)
      mr_commons.makedirs(modeltest_fasta_output_dir)
      writer.write("modeltest_" + name + " ") 
      writer.write(str(op.modeltest_cores) + " " + str(msa.taxa * msa.sites))
      writer.write(" -i ")
      writer.write(msa.path)
      writer.write(" -t mp ")
      writer.write(" -o " +  os.path.join(modeltest_results, name, name))
      writer.write(" " + msa.modeltest_arguments + " ")
      writer.write("\n")
  mr_scheduler.run_mpi_scheduler(library, scheduler, commands_file, run_path, cores)  

def parse_modeltest_results(modeltest_criteria, msas, output_dir):
  """ Parse the results from the MPI scheduler run to get the best-fit model
  for each MSA """
  run_path = os.path.join(output_dir, "modeltest_run")
  modeltest_results = os.path.join(run_path, "results")
  models = {}
  for name, msa in msas.items():
    if (not msa.valid):
      continue
    modeltest_outfile = os.path.join(modeltest_results, name, name + ".out")  
    with open(modeltest_outfile) as reader:
      read_next_model = False
      for line in reader.readlines():
        if (line.startswith("Best model according to " + modeltest_criteria)):
            read_next_model = True
        if (read_next_model and line.startswith("Model")):
          model = line.split(" ")[-1][:-1]
          msa.set_model(model)
          if (not model in models):
            models[model] = 0
          models[model] += 1
          break
  # write a summary of the models
  with open(os.path.join(run_path, "summary.txt"), "w") as writer:
    for model, count in sorted(models.items(), key=lambda x: x[1], reverse=True):
      writer.write(model + " " + str(count) + "\n")


