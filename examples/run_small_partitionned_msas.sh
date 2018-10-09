
multiraxml=../pargenes/pargenes.py
output=results/small_partitionned_msas
msa_directory=data/small/fasta_files/
cores=4
per_msa_raxml_options=data/small/per_msa_partitions.txt
filter=data/small/msa_filter.txt

rm -rf ${output}
python ${multiraxml} -a ${msa_directory} -o ${output} -c ${cores} --per-msa-raxml-parameters ${per_msa_raxml_options} --msa-filter ${filter}

