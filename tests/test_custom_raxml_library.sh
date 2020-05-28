
if [ "$#" -ne 1 ]; then
  echo "Syntax: test_custom_raxml_library.sh path_to_raxml_library"
  echo "To try with the default library used by ParGenes, use"
  echo "../raxml-ng/bin/raxml-ng-mpi.so"
  exit 1
fi

pargenes=../pargenes/pargenes-hpc.py
output=custom_raxml_test_output
library=$1

rm -rf $output
python $pargenes -a smalldata/fasta_files -o $output -r smalldata/raxml_global_options.txt -c 2 -s 3 -p 3 --raxml-binary $library

if [ "$?" -ne 0 ]; then
  echo ""
  echo "Test KO with library $library"
else
  echo ""
  echo "Test OK with library $library"
fi
