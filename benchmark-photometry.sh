authors=('ChatGPT' 'Claude' 'Gemini2.5' 'Human')

languages=('c' 'rust' 'python')

rm times-photometry.txt
cd Photometry
for author in "${authors[@]}"
do
	if [ -d "$author" ] 
	then
  		cd $author
		for lang in "${languages[@]}"
		do
			if grep "$lang:" Makefile | grep -v "#";
			then
				if make $lang;
				then
					echo "photometry $version $author $lang" >> ../../times-photometry.txt
					for cnt in {1..7}
					do
						{ time make "run-$lang" > /dev/null ; } 2>> ../../times-photometry.txt
					done
				fi
			fi
		done
		cd ..
	fi
done
