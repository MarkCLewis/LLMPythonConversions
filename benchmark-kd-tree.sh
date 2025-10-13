authors=('ChatGPT' 'Claude' 'Gemini2.5' 'Human')

languages=('c' 'rust' 'python')

VersionPattern="Version*"

rm times-kd-tree.txt
cd kd-tree
for version in $VersionPattern
do
	cd $version
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
						echo "kd-tree $version $author $lang" >> ../../../times-kd-tree.txt
						for cnt in {1..7}
						do
							{ time make "run-$lang" > /dev/null ; } 2>> ../../../times-kd-tree.txt
						done
					fi
				fi
			done
			cd ..
		fi
	done
	cd ..
done
