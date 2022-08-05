import os
import scheduler
import commons


def compute_constrain(msas, samples, raxml_library, scheduler_mode, run_path, cores, op):
  parsi_run_path = os.path.join(run_path, "parsimony")
  parsi_commands_file = os.path.join(parsi_run_path, "command.txt")
  parsi_results = os.path.join(parsi_run_path, "results")
  commons.makedirs(parsi_results)
  with open(parsi_commands_file, "w") as writer:
    for name, msa in msas.items():
      prefix = os.path.join(parsi_results, name)
      if (not msa.valid):
        continue
      msa_path = msa.binary_path
      if (msa_path == "" or msa_path == None):
        msa_path = msa.path
      msa_size = 1
      writer.write("parsi_" + name + " 1 1")
      writer.write(" --msa " + msa_path + " ")
      writer.write(" --prefix " + os.path.abspath(prefix))
      if (scheduler_mode != "fork"):
        writer.write(" --threads 1 ")
      writer.write(" --tree pars{" + str(samples) + "} ")
      writer.write(" --start") 
      writer.write(" --model " + msa.get_model())
      writer.write("\n")
  scheduler.run_scheduler(raxml_library, scheduler_mode, "--threads", parsi_commands_file, parsi_run_path, cores, op)  
     

  consensus_run_path = os.path.join(run_path, "consensus")
  consensus_commands_file = os.path.join(consensus_run_path, "command.txt")
  consensus_results = os.path.join(consensus_run_path, "results")
  commons.makedirs(consensus_results)
  with open(consensus_commands_file, "w") as writer:
    for name, msa in msas.items():
      prefix = os.path.join(consensus_results, name)
      if (not msa.valid):
        continue
      trees = os.path.join(parsi_results, name + ".raxml.startTree")
      writer.write("consensus_" + name + " 1 1")
      writer.write(" --prefix " + os.path.abspath(prefix))
      if (scheduler_mode != "fork"):
        writer.write(" --threads 1 ")
      writer.write(" --tree " + trees)
      writer.write(" --consense STRICT") 
      writer.write("\n")
  scheduler.run_scheduler(raxml_library, scheduler_mode, "--threads", consensus_commands_file, consensus_run_path, cores, op)  


def get_constraint(op, msa_name):
  return os.path.join(op.output_dir, "constrain_run", "consensus", "results", msa_name + ".raxml.consensusTreeSTRICT")
