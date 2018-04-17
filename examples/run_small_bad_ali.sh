
multiraxml=../multi-raxml/multi-raxml.py
output=results/small_bad_ali
msa_directory=data/small/fasta_filesdflj/
raxml_global_options=data/small/raxml_global_options.txt
cores=4

rm -rf ${output}
python ${multiraxml} -a ${msa_directory} -o ${output} -r ${raxml_global_options} -c ${cores}

