benchmarks=('k-nuclcotide'  'spectral-norm'
  'binary-trees'    'mandelbrot' 
  'fannkuch-redux'  'nbody'         'regex-redux'
  'fasta'           'pidigits'      'reverse-complement'
)

VersionPattern="Version*"

for bench in "${benchmarks[@]}"
do
	cd $bench
	for version in $VersionPattern
	do
		cd $version
			if [ ! -d "Human" ] 
			then
				mkdir Human
			fi
			mv *.py Human/
  		cd ..
	done
	cd ..
done
