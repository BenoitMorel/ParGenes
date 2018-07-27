
multiraxml=../pargenes/pargenes.py
output=results/small_filter
msa_directory=data/small/fasta_files/
raxml_global_options=data/small/raxml_global_options.txt
cores=4
filter=data/small/msa_filter.txt

rm -rf ${output}
python ${multiraxml} -a ${msa_directory} -o ${output} -r ${raxml_global_options} -c ${cores}  --msa-filter ${filter}

