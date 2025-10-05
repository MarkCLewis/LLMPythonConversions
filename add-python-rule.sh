benchmarks=('k-nuclcotide'  'spectral-norm'
  'binary-trees'    'mandelbrot' 
  'fannkuch-redux'  'nbody'         'regex-redux'
  'fasta'           'pidigits'      'reverse-complement'
  'kd-tree'         'Photometry'
)

VersionPattern="Version*"

for bench in "${benchmarks[@]}"
do
	cd $bench
	for version in $VersionPattern
	do
		cd $version
		for author in "${authors[@]}"
		do
  			cd Human
			make c
			make run-c > ../output.txt
			cd ..
		done
  		cd ..
	done
	cd ..
done
