
multiraxml=../multi-raxml/multi-raxml.py
output=results/small_disable_sort
msa_directory=data/small/fasta_files/
raxml_global_options=data/small/raxml_global_options.txt
cores=4

rm -rf ${output}
python ${multiraxml} -a ${msa_directory} -o ${output} -r ${raxml_global_options} -c ${cores} --experiment-disable-jobs-sorting

