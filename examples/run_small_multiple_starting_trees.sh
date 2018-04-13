
multiraxml=../multi-raxml/multi-raxml.py
output=results/small_multiple_starting_trees
msa_directory=data/small/fasta_files/
raxml_global_options=data/small/raxml_global_options.txt
cores=4
starting_trees=3

rm -rf ${output}
python ${multiraxml} -a ${msa_directory} -o ${output} -r ${raxml_global_options} -c ${cores} -s ${starting_trees}

