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
	rm times-c.txt
	for version in $VersionPattern
	do
		cd $version
		for author in "${authors[@]}"
		do
			if [ -d "$author" ] 
			then
  				cd $author
				make c
				echo $bench $version $author $file # >> ../times-c.txt
				for cnt in {1..7}
				do
					{ time make run-c ; } 2>> ../times-c.txt
				done
				cd ..
			fi
		done
  		cd ..
	done
	cd ..
done
