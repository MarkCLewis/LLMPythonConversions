benchmarks=('k-nuclcotide'  'spectral-norm'
  'binary-trees'    'mandelbrot' 
  'fannkuch-redux'  'nbody'         'regex-redux'
  'fasta'           'pidigits'      'reverse-complement'
)
authors=('ChatGPT' 'Claude' 'Gemini2.5' 'Human')

VersionPattern="Version*"

for bench in "${benchmarks[@]}"
do
	cd $bench
	for version in $VersionPattern
	do
		cd $version
		for author in "${authors[@]}"
		do
			if [ -d "$author" ] 
			then
  				cd $author
				echo "$bench $version $author"
				if grep "c:" Makefile | grep -v "#";
				then
					make c
				fi
				if grep "rust:" Makefile | grep -v "#";
				then
					make rust
				fi
				cd ..
			fi
		done
  		cd ..
	done
	cd ..
done
