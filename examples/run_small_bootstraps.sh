
multiraxml=../pargenes/pargenes.py
output=results/small_bootstraps
msa_directory=data/small/fasta_files/
raxml_global_options=data/small/raxml_global_options.txt
cores=4
bootstraps=5

rm -rf ${output}
python3 ${multiraxml} -a ${msa_directory} -o ${output} -r ${raxml_global_options} -c ${cores} -b ${bootstraps}

