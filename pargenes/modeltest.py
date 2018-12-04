import os
import commons
import scheduler
import logger
import report

def run(msas, output_dir, library, run_path, op): 
  """ Use the MPI scheduler to run modeltest on all the MSAs"""
  cores = op.cores
  run_path = os.path.join(output_dir, "modeltest_run")
  commands_file = os.path.join(run_path, "modeltest_command.txt")
  modeltest_results = os.path.join(run_path, "results")
  commons.makedirs(modeltest_results)
  if (op.modeltest_cores < 4):
    logger.error("The number of cores per modeltest job should at least be 4")
    report.report_and_exit(op.output_dir, 1)
  with open(commands_file, "w") as writer:
    for name, msa in msas.items():
      if (not msa.valid):
        continue
      modeltest_fasta_output_dir = os.path.join(modeltest_results, name)
      commons.makedirs(modeltest_fasta_output_dir)
      writer.write("modeltest_" + name + " ") 
      writer.write(str(op.modeltest_cores) + " " + str(msa.taxa * msa.per_taxon_clv_size))
      writer.write(" -i ")
      writer.write(msa.path)
      writer.write(" -t mp ")
      writer.write(" -o " +  os.path.join(modeltest_results, name, name))
      writer.write(" " + msa.modeltest_arguments + " ")
      writer.write("\n")
  scheduler.run_mpi_scheduler(library, op.scheduler, commands_file, run_path, cores, op)  

def get_model_from_log(log_file, modeltest_criteria):
  with open(log_file) as reader:
    read_next_model = False
    for line in reader.readlines():
      if (line.startswith("Best model according to " + modeltest_criteria)):
          read_next_model = True
      if (read_next_model and line.startswith("Model")):
        model = line.split(" ")[-1][:-1]
        return model
  return None

def parse_modeltest_results(modeltest_criteria, msas, output_dir):
  """ Parse the results from the MPI scheduler run to get the best-fit model
  for each MSA """
  run_path = os.path.join(output_dir, "modeltest_run")
  modeltest_results = os.path.join(run_path, "results")
  models = {}
  for name, msa in msas.items():
    if (not msa.valid):
      continue
    try:
      modeltest_outfile = os.path.join(modeltest_results, name, name + ".out")  
      model = get_model_from_log(modeltest_outfile, modeltest_criteria)
      if (model == None):
        msa.valid = False
        continue
      msa.set_model(model)
      if (not model in models):
         models[model] = 0
         models[model] += 1
    except:
      msa.valid = False
      continue
    # write a summary of the models
  with open(os.path.join(run_path, "summary.txt"), "w") as writer:
    for model, count in sorted(models.items(), key=lambda x: x[1], reverse=True):
      writer.write(model + " " + str(count) + "\n")


