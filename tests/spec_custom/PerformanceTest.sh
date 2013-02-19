#!/bin/bash

function help {
    
    echo "Script to run the PerformanceTest multiple times."    
    echo -e "\nUsage: PerformanceTest.sh [Options]"    
    echo -e "\nOptions" 
    echo "-o=FileName : Output FileName. Default PerformanceTest.html"          

}


FILENAME="PerformanceTest.html"
RECOMPILE="YES"
COUNTOVERFLOWS="YES"

#Read the arguments
for arg in "$@"
do
    LENGTH=`expr length $arg`
    LENGTH=`expr $LENGTH - 2`    
    case "$arg" in
        -[oO]=*)   FILENAME=`expr substr $arg 4 $LENGTH`
                    ;;            
        --help) help
                exit
                ;;
        -no-count-overflows) COUNTOVERFLOWS="NO"
                ;; 
        -no-recompile) RECOMPILE="NO"
                ;;                
        *)      echo "Invalid Option. Use --help option to get help."
                exit
                ;;
    esac
done

if [ "$RECOMPILE" == "YES" ]; then
	
	make clean
	make TEST=CompileAll
	
fi

# Diretório inicial: /work/lnt-llvm/llvm-3.0/projects/test-suite/External/SPEC

# Objetivo: iterar por todos os diretórios testando se há um diretório Output
# dois níveis abaixo do diretório inicial.

# Se houver, o arquivo *.linked.rbc.m2r.rbc será compilado para dois executáveis:
# um executável com instrumentação e um sem instrumentação.
# Os dois programas serão executados e o tempo de execução será coletado para comparação.

export PATH=$PATH:/work/lnt-llvm/llvm-3.0/Debug+Asserts/bin/:/work/lnt-llvm/llvm-3.0/Debug+Asserts/lib/

CURDIR=$(cd $(dirname "$0"); pwd)

RESULTFILE="$CURDIR/$FILENAME"

TIMEFORMAT="%U"

TMPFILE=`mktemp`
TMPFILE2=`mktemp`

#Level 1: Benchmarks (CFP95, CFP2000, CFP2006, etc...)
for Level1 in $CURDIR/*
do
	
	if [ -d $Level1 ]; then
		
		#Level 2: Benchmark programs
		for Level2 in $Level1/*
		do
	
			if [ -d $Level2 ]; then
				
				#If the program was successful compiled, there is a folder named Output
				#inside the program folder
				if [ -d $Level2/Output ] ; then
					
                    CopyFilesToCurrentFolder="NO"
                    AdditionalFilesToCopy=""

					# Argument list for each program
					#
					# These nested IFs aren't beautiful, but there is a few programs,
					# so it was easier to do this way than do in a more elegant way
					if [[ "$Level2" == *"433.milc"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/433.milc/data/test/input/su3imp.in "
						OUTPUTFILE=""
						STDOUTFILE="milc.test.out"
						STDERRFILE="milc.test.err"
						ARGUMENTS=" < $INPUTFILE > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"444.namd"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/444.namd/data/all/input/namd.input"
						ARGUMENTS=" --input $INPUTFILE --iterations 1 --output namd.out >  2> namd.ref.err "
						OUTPUTFILE="namd.out"
						STDOUTFILE="namd.all.out"
						STDERRFILE="namd.all.err"						
						ARGUMENTS=" --input $INPUTFILE --iterations 1 --output $OUTPUTFILE > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"447.dealII"* ]]; then
						OUTPUTFILE=""
						STDOUTFILE="dealII.test.out"
						STDERRFILE="dealII.test.err"						
						ARGUMENTS=" 3 > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"450.soplex"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/450.soplex/data/test/input/test.mps"
						OUTPUTFILE=""
						STDOUTFILE="soplex.test.out"
						STDERRFILE="soplex.test.err"					   
						ARGUMENTS="-m1200 $(basename "$INPUTFILE") > $STDOUTFILE 2> $STDERRFILE "
                        CopyFilesToCurrentFolder="YES"
					
					elif [[ "$Level2" == *"470.lbm"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/470.lbm/data/test/input/100_100_130_cf_a.of"
						OUTPUTFILE=""
						STDOUTFILE="lbm.ref.out"
						STDERRFILE="lbm.ref.err"						
						ARGUMENTS="1000 reference.dat 0 0 $INPUTFILE > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"400.perlbench"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/400.perlbench/data/all/input/diffmail.pl"
						OUTPUTFILE=""
						STDOUTFILE="perlbench.all.diffmail.out"
						STDERRFILE="dperlbench.ref.diffmail.err"						
						ARGUMENTS=" -I./lib $INPUTFILE 4 800 10 17 19 300 > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"401.bzip2"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/401.bzip2/data/all/input/input.program"
						OUTPUTFILE=""
						STDOUTFILE="bzip2.all.program.out"
						STDERRFILE="bzip2.all.program.err"						
						ARGUMENTS=" $INPUTFILE 280 > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"429.mcf"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/429.mcf/data/test/input/inp.in"
						OUTPUTFILE=""
						STDOUTFILE="mcf.test.out"
						STDERRFILE="mcf.test.err"						
						ARGUMENTS=" $INPUTFILE > $STDOUTFILE 2> $STDERRFILE "
					
#					elif [[ "$Level2" == *"403.gcc"* ]]; then
#						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/403.gcc/data/test/input/cccp.i"
#						OUTPUTFILE=""
#						STDOUTFILE="gcc.test.cccp.out"
#						STDERRFILE="gcc.test.cccp.err"						
#						ARGUMENTS=" $INPUTFILE -o cccp.s > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"445.gobmk"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/445.gobmk/data/test/input/connection_rot.tst"
						OUTPUTFILE=""
						STDOUTFILE="gobmk.connection_rot.out"
						STDERRFILE="gobmk.connection_rot.err"						
						ARGUMENTS=" --quiet --mode gtp < $INPUTFILE > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"456.hmmer"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/456.hmmer/data/test/input/bombesin.hmm"
						OUTPUTFILE=""
						STDOUTFILE="hmmer.test.bombesin.out"
						STDERRFILE="hmmer.test.bombesin.err"	
						ARGUMENTS=" --fixed 0 --mean 325 --num 45000 --sd 200 --seed 0 $(basename "$INPUTFILE") > $STDOUTFILE 2> $STDERRFILE "
                        CopyFilesToCurrentFolder="YES"
					
					elif [[ "$Level2" == *"458.sjeng"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/458.sjeng/data/test/input/test.txt"
						OUTPUTFILE=""
						STDOUTFILE="sjeng.test.out"
						STDERRFILE="sjeng.test.err"						
						ARGUMENTS=" $INPUTFILE > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"462.libquantum"* ]]; then
						OUTPUTFILE=""
						STDOUTFILE="libquantum.test.out"
						STDERRFILE="libquantum.test.err"					
						ARGUMENTS=" 33 5 > $STDOUTFILE 2> $STDERRFILE "
					
					elif [[ "$Level2" == *"464.h264ref"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/464.h264ref/data/test/input/foreman_test_encoder_baseline.cfg"
						OUTPUTFILE=""
						STDOUTFILE="h264ref.test.foreman_baseline.out"
						STDERRFILE="h264ref.test.foreman_baseline.err"						
						ARGUMENTS=" -d $(basename "$INPUTFILE") > $STDOUTFILE 2> $STDERRFILE "
                        CopyFilesToCurrentFolder="YES"
                        AdditionalFilesToCopy="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/464.h264ref/data/all/input/foreman_qcif.yuv"
					
					elif [[ "$Level2" == *"471.omnetpp"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/471.omnetpp/data/test/input/omnetpp.ini"
						OUTPUTFILE=""
						STDOUTFILE="omnetpp.test.log"
						STDERRFILE="omnetpp.test.err"						
						ARGUMENTS=" $(basename "$INPUTFILE") > $STDOUTFILE 2> $STDERRFILE "
                        CopyFilesToCurrentFolder="YES"
					
					elif [[ "$Level2" == *"473.astar"* ]]; then
						INPUTFILE="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/473.astar/data/test/input/lake.cfg"
						OUTPUTFILE=""
						STDOUTFILE="astar.test.lake.out"
						STDERRFILE="astar.test.lake.err"						
						ARGUMENTS=" $(basename "$INPUTFILE") > $STDOUTFILE 2> $STDERRFILE "
                        CopyFilesToCurrentFolder="YES"
                        AdditionalFilesToCopy="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/473.astar/data/test/input/lake.bin"
					
					elif [[ "$Level2" == *"483.xalancbmk"* ]]; then
						INPUTFOLDER="/work/lnt-llvm/SPEC/speccpu2006/benchspec/CPU2006/483.xalancbmk/data/test/input"
						INPUTFILE1="$INPUTFOLDER/test.xml"
						INPUTFILE2="$INPUTFOLDER/xalanc.xsl"
						OUTPUTFILE=""
						STDOUTFILE="xalancbmk.test.out"
						STDERRFILE="xalancbmk.test.err"						
						ARGUMENTS=" -v $INPUTFILE1 $INPUTFILE2 > $STDOUTFILE 2> $STDERRFILE "
					
					else
						INPUTFILE=""
						OUTPUTFILE=""
						STDOUTFILE=""
						STDERRFILE=""
						ARGUMENTS=""
					fi 

					for File in $Level2/Output/*.linked.rbc.m2r.rbc
					do
						
						if [ -f $File ] && [ "$ARGUMENTS" != "" ]; then

							echo -e "<tr>" >> $TMPFILE							
							
							#Column 1: Program Name
							ProgramName=$(basename "$Level2")
							echo "<td>$ProgramName</td>" >> $TMPFILE							
							
							Filename=$(basename "$File")
							
							cd $Level2/Output/
							
                            # Some programs requires write rights. To guarantee that we have write rights
                            # we will copy these files to the current folder.
                            if [ $CopyFilesToCurrentFolder == "YES" ]; then
                                for CopyFile in $INPUTFILE $AdditionalFilesToCopy
                                do
                                    LocalFileName=$(basename "$CopyFile")
                                    cp $CopyFile $LocalFileName
                                done
                            fi

                            if [ "$RECOMPILE" == "YES" ]; then
		                        echo "Preparing $Filename file..."
								llc $Filename -o $Filename.s
								g++ $Filename.s -o $Filename.exe
							fi
							
							echo "Executing $Filename.exe..."
							CMD="/usr/bin/time -f $TIMEFORMAT -o $TMPFILE2 ./$Filename.exe $ARGUMENTS"							
							date +%T
							echo $CMD
                            eval $CMD
							
							#Column 2: Original Time
							echo "<td>" >> $TMPFILE
							cat $TMPFILE2 >> $TMPFILE
							echo "</td>" >> $TMPFILE							
							
							
							#Backup of the output files
							if [ -f $OUTPUTFILE ]; then
								TMPOUTPUTFILE=`mktemp`
								mv $OUTPUTFILE $TMPOUTPUTFILE
							else
								TMPOUTPUTFILE=""
							fi
							
							if [ -f $STDOUTFILE ]; then
								TMPSTDOUTFILE=`mktemp`
								mv $STDOUTFILE $TMPSTDOUTFILE
							else
								TMPSTDOUTFILE=""
							fi
							
							if [ -f $STDERRFILE ]; then
								TMPSTDERRFILE=`mktemp`
								mv $STDERRFILE $TMPSTDERRFILE
							else
								TMPSTDERRFILE=""
							fi
							
							################# Execution 02 >> Instrumentation without RA prunning ########################						
							
							if [ "$RECOMPILE" == "YES" ]; then							
								echo "Preparing $Filename instrumented (without RA) file..."
								opt -load uSSA.so -load RangeAnalysis.so -load OverflowDetect.so \
									-insert-trunc-checks -insert-stderr-fprintfs=false -overflow-detect $Filename > $Filename.inst1.bc
								llc $Filename.inst1.bc -o $Filename.inst1.s
								g++ $Filename.inst1.s -o $Filename.inst1.exe
							fi

							echo "Executing $Filename.inst1.exe (without RA)..."												
							CMDInst="/usr/bin/time -f $TIMEFORMAT -o $TMPFILE2 ./$Filename.inst1.exe $ARGUMENTS"	
							date +%T
							echo $CMDInst
                            eval $CMDInst
							
							#Column 3: Instrumented Time
							echo "<td>" >> $TMPFILE
							cat $TMPFILE2 >> $TMPFILE
							echo "</td>" >> $TMPFILE						
							
#							if [ "$RECOMPILE" == "YES" ]; then	
#								echo "Preparing $Filename instrumented (without RA, with fprintf) file..."
#								opt -load uSSA.so -load RangeAnalysis.so -load OverflowDetect.so \
#									-insert-trunc-checks -overflow-detect $Filename > $Filename.inst2.bc
#								llc $Filename.inst2.bc -o $Filename.inst2.s
#								g++ $Filename.inst2.s -o $Filename.inst2.exe	
#							fi	
#							
#							CMDInst="/usr/bin/time -f $TIMEFORMAT -o $TMPFILE2 ./$Filename.inst2.exe $ARGUMENTS"					
#							date +%T
#							echo "Executing $Filename.inst2.exe (without RA, with fprintf)..."	
#							eval $CMDInst
#							
#							#Column 4: Overflow Occurrences
#							echo "<td>" >> $TMPFILE
#							grep "Overflow occurred" $STDERRFILE | wc -l >> $TMPFILE		
#							echo "</td>" >> $TMPFILE
							

							# Evaluating the output files to check if the instrumented executable
							# produces the same exit of the original executable
							
							if [ -f $TMPOUTPUTFILE ]; then
								RESULT=`diff --brief $TMPOUTPUTFILE $OUTPUTFILE |wc -l`
							else
								if [ -f $OUTPUTFILE ]; then
									RESULT="1"
								else
									RESULT="0"
								fi
							fi
							
							if [ $RESULT == "0" ]; then
								if [ -f $TMPSTDOUTFILE ]; then
									RESULT=`diff --brief $TMPSTDOUTFILE $STDOUTFILE |wc -l`
								else
									if [ -f $STDOUTFILE ]; then
										RESULT="1"
									else
										RESULT="0"
									fi
								fi
							fi
							
							#Column 5: Equal Outputs
							echo "<td>" >> $TMPFILE
							if [ $RESULT == "0" ]; then
								echo "YES" >> $TMPFILE
							else
								echo "NO" >> $TMPFILE							
							fi
							echo "</td>" >> $TMPFILE
							
							################# Execution 03 >> Instrumentation with RA prunning ########################
							
							if [ "$RECOMPILE" == "YES" ]; then
								echo "Preparing $Filename instrumented file (with vSSA + RA prunning)..."
								opt -load uSSA.so -load vSSA.so -load RangeAnalysis.so -load OverflowDetect.so \
									-vssa -insert-trunc-checks -insert-stderr-fprintfs=false -use-ra-prunning \
									-overflow-detect $Filename > $Filename.inst3.bc
								llc $Filename.inst3.bc -o $Filename.inst3.s
								g++ $Filename.inst3.s -o $Filename.inst3.exe
							fi

							echo "Executing $Filename.inst3.exe (with vSSA + RA prunning)..."												
							CMDInst="/usr/bin/time -f $TIMEFORMAT -o $TMPFILE2 ./$Filename.inst3.exe $ARGUMENTS"	
							date +%T							
							echo $CMDInst
                            eval $CMDInst
							
							#Column 6: Instrumented Time
							echo "<td>" >> $TMPFILE
							cat $TMPFILE2 >> $TMPFILE
							echo "</td>" >> $TMPFILE
							
#							if [ "$RECOMPILE" == "YES" ]; then
#								echo "Preparing $Filename instrumented file (with fprintf + vSSA + RA prunning)..."
#								opt -load uSSA.so -load vSSA.so -load RangeAnalysis.so -load OverflowDetect.so \#
#									-vssa -insert-trunc-checks -use-ra-prunning -overflow-detect $Filename > $Filename.inst4.bc
#								llc $Filename.inst4.bc -o $Filename.inst4.s
#								g++ $Filename.inst4.s -o $Filename.inst4.exe
#							fi
#
#							echo "Executing $Filename.inst4.exe (with fprintf + vSSA + RA prunning)..."	
#							CMDInst="/usr/bin/time -f $TIMEFORMAT -o $TMPFILE2 ./$Filename.inst4.exe $ARGUMENTS"	
#							date +%T
#							echo $CMDInst
#                            eval $CMDInst							
#
#							#Column 7: Overflow Occurrences
#							echo "<td>" >> $TMPFILE
#							grep "Overflow occurred" $STDERRFILE | wc -l >> $TMPFILE		
#							echo "</td>" >> $TMPFILE
							

							# Evaluating the output files to check if the instrumented executable
							# produces the same exit of the original executable
							
							if [ -f $TMPOUTPUTFILE ]; then
								RESULT=`diff --brief $TMPOUTPUTFILE $OUTPUTFILE |wc -l`
							else
								if [ -f $OUTPUTFILE ]; then
									RESULT="1"
								else
									RESULT="0"
								fi
							fi
							
							if [ $RESULT == "0" ]; then
								if [ -f $TMPSTDOUTFILE ]; then
									RESULT=`diff --brief $TMPSTDOUTFILE $STDOUTFILE |wc -l`
								else
									if [ -f $STDOUTFILE ]; then
										RESULT="1"
									else
										RESULT="0"
									fi
								fi
							fi
							
							#Column 8: Equal Outputs
							echo "<td>" >> $TMPFILE
							if [ $RESULT == "0" ]; then
								echo "YES" >> $TMPFILE
							else
								echo "NO" >> $TMPFILE							
							fi
							echo "</td>" >> $TMPFILE							
							
							################# Execution 04 >> Instrumentation with uSSA + RA prunning + Aborts ########################
							
							if [ "$RECOMPILE" == "YES" ]; then
								echo "Preparing $Filename instrumented file (with vSSA + uSSA + RA prunning + aborts)..."
								opt -load uSSA.so -load vSSA.so -load RangeAnalysis.so -load OverflowDetect.so \
									-vssa -insert-trunc-checks -ussa -insert-ovf-aborts -use-ra-prunning -overflow-detect \
									$Filename > $Filename.inst5.bc
									
								llc $Filename.inst5.bc -o $Filename.inst5.s
								g++ $Filename.inst5.s -o $Filename.inst5.exe
							fi

							echo "Executing $Filename.inst5.exe (with vSSA + uSSA + RA prunning + aborts)..."		
							CMDInst="/usr/bin/time -f $TIMEFORMAT -o $TMPFILE2 ./$Filename.inst5.exe $ARGUMENTS"	
							date +%T
							echo $CMDInst
                            eval $CMDInst
							
							#Column 9: Instrumented Time
							echo "<td>" >> $TMPFILE
							cat $TMPFILE2 >> $TMPFILE
							echo "</td>" >> $TMPFILE

							cd -					

							echo -e "</tr>" >> $TMPFILE	
							
						fi					

					done
									
				fi
				
			fi
			
		done	
	fi	

done

echo "Generating results..."

echo -e "<html><body>\n

		<table border='1' cellspacing='1' cellpadding='0'>\n

		<tr bgcolor=\"aqua\">\n

		<td><center><b>Program Name</b></center></td>\n
		<td><center><b>Original Time</b></center></td>\n
		<td><center><b>Instrumented Time (without RA prunning)</b></center></td>\n
		<td><center><b>Equal Outputs</b></center></td>\n		
		<td><center><b>Instrumented Time (with vSSA + RA prunning)</b></center></td>\n	
		<td><center><b>Equal Outputs</b></center></td>\n		
		<td><center><b>Instrumented Time (with vSSA + uSSA + RA prunning + aborts)</b></center></td>\n				

		</tr>\n" > $RESULTFILE

cat $TMPFILE >> $RESULTFILE

echo -e "</table>\n
		</body></html>" >> $RESULTFILE

rm -f /tmp/tmp.*


