#!/bin/bash

function help {
    
        echo "Script to run the PerformanceTest multiple times."    
        echo -e "\nUsage: PerformanceTests.sh [Options]"    
        echo -e "\nOptions" 
        echo "-N :  Number of times the PerformanceTest will be executed. Default 1."       
        echo "-o=FileName : Output FileName. Default PerformanceTest.html"          
                            
}

#Default Values
NUM_ITERATIONS=1
FILENAME="PerformanceTest.html"

#Read the arguments
for arg in "$@"
do
    LENGTH=`expr length $arg`
    LENGTH=`expr $LENGTH - 2`    
    case "$arg" in
        -[N]*)    NUM_ITERATIONS=`expr substr $arg 3 $LENGTH`
                   ;;
        -[oO]=*)   FILENAME=`expr substr $arg 4 $LENGTH`
                   ;;            
        -no-recompile) RECOMPILEFLAG="-no-recompile"
					;;
        --help)    help
                   exit
                   ;;                        
        *)         echo "Invalid Option. Use --help option to get help."
                   exit
                   ;;
    esac
done

#Preparing FILENAME
INDEX=`expr index $FILENAME "."`

if [ $INDEX == "0" ]; then
    FILENAME=$FILENAME.html
fi

INDEX=`expr index $FILENAME "."`

#Run the script N times
for ((i = 1; i <= $NUM_ITERATIONS; i++)); do

    #Preparing the filename to be passed as the argument
    PREFIX=`expr substr $FILENAME 1 $INDEX`
    LENGTH=`expr length $FILENAME`
    LENGTH=`expr $LENGTH - $INDEX` 
    LENGTH=`expr $LENGTH + 1` 
    SUFFIX=`expr substr $FILENAME $INDEX $LENGTH`

    bash PerformanceTest.sh -o=$PREFIX$i$SUFFIX $RECOMPILEFLAG
    
    RECOMPILEFLAG="-no-recompile"
done

