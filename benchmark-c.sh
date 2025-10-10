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
				if grep "c:" Makefile | grep -v "#";
				then
					if make c;
					then
						echo $bench $version $author $file >> ../../../times-c.txt
						for cnt in {1..7}
						do
							{ time make run-c > /dev/null ; } 2>> ../../../times-c.txt
						done
					fi
				fi
				cd ..
			fi
		done
  		cd ..
	done
	cd ..
done
