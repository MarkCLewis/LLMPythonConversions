benchmarks=('k-nuclcotide'  'spectral-norm'
  'binary-trees'    'mandelbrot' 
  'fannkuch-redux'  'nbody'         'regex-redux'
  'fasta'           'pidigits'      'reverse-complement'
)
authors=('ChatGPT' 'Claude' 'Human')

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
				cp ./Gemini2.5/Makefile ./$author/
			fi
		done
  		cd ..
	done
	cd ..
done
