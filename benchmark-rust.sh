benchmarks=('k-nuclcotide'  'spectral-norm'
  'binary-trees'    'mandelbrot' 
  'fannkuch-redux'  'nbody'         'regex-redux'
  'fasta'           'pidigits'      'reverse-complement'
)
authors=('ChatGPT' 'Claude' 'Gemini2.5' 'Human')

VersionPattern="Version*"

rm times-rust.txt
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
				if grep "rust:" Makefile | grep -v "#";
				then
					if make rust;
					then
						echo "$bench $version $author rust" >> ../../../times-rust.txt
						for cnt in {1..7}
						do
							{ time make run-rust > /dev/null ; } 2>> ../../../times-rust.txt
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
