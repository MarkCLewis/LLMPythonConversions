benchmarks=('k-nuclcotide'  'spectral-norm'
  'binary-trees'    'mandelbrot' 
  'fannkuch-redux'  'nbody'         'regex-redux'
  'fasta'           'pidigits'      'reverse-complement'
)
authors=('Human')

VersionPattern="Version*"

for bench in "${benchmarks[@]}"
do
	cd $bench
	rm times-python.txt
	for version in $VersionPattern
	do
		cd $version
		for author in "${authors[@]}"
		do
			if [ -d "$author" ] 
			then
  				cd $author
				echo $bench $version $author $file >> ../../../times-python.txt
				for cnt in {1..7}
				do
					{ time make run-python > /dev/null ; } 2>> ../../../times-python.txt
				done
				cd ..
			fi
		done
  		cd ..
	done
	cd ..
done
