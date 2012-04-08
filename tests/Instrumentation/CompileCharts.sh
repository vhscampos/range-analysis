#!/bin/bash

rm -f RAEstimatedValues.Benchmark.txt

#Appends every RAEstimatedValues*.txt into one single file, named RAEstimatedValues.txt 
for file in RAEstimatedValues*.txt ; do

	#echo "$file"
	cat $file >> RAEstimatedValues.Benchmark.txt

done

#Joins the RAHashNames and RAHashValues of each module, into its respective RARealValues.
for file in RAHashNames*.txt ; do

	RAHashValues=`echo $file | sed -e "s/RAHashNames/RAHashValues/g"`
	RARealValues=`echo $file | sed -e "s/RAHashNames/RARealValues/g"`
	
	awk 'ARGIND == 1 {vars[$2] = $1} ARGIND == 2 { if(vars[$1]!="") { print vars[$1], $2, $3 } }' $file $RAHashValues > $RARealValues

done


rm -f RARealValues.Benchmark.txt

#Appends every RARealValues*.txt into one single file, named RARealValues.txt 
for file in RARealValues*.txt ; do

	cat $file >> RARealValues.Benchmark.txt

done


for file in RARealValues*.txt ; do

	RARealValues=$file
	RAEstimatedValues=`echo $file | sed -e "s/RARealValues/RAEstimatedValues/g"`
	RAAnalysisSource=`echo $file | sed -e "s/RARealValues/RAAnalysisSource/g"`	

	#Joins the files RAEstimatedValues and RARealValues in order to produce the base file of the evaluation of the quality of Range Analysis.
	awk 'ARGIND == 1 {Emin[$1] = $2; Emax[$1] = $3} ARGIND == 2 { print $1, $2, $3, Emin[$1], Emax[$1] }' $RAEstimatedValues $RARealValues > $RAAnalysisSource

done

rm -f RAChartSource*.txt

#For each analysis file, generate one file to be read by GNUPLOT
for file in RAAnalysisSource*.txt ; do

	RAAnalysisSource=$file
	RAChartSource=`echo $file | sed -e "s/RAAnalysisSource/RAChartSource/g"`	

	awk '	
	     	function abs(x){
	     		if(x<0)
	     			return -x
	     		else
	     			return x
	     	}
			{
				total+= 1
				
				Rlower = abs($2)
				Elower = abs($4)
				Ldiff = abs(Rlower-Elower)
				
				if(Ldiff <= 2)
					O1_lower++
				else if (Ldiff <= Rlower)
					ON_lower++
				else if (Ldiff <= Rlower*Rlower)
					ON2_lower++
				else
					OWorse_lower++				
				
				
				Rupper = abs($3)
				Eupper = abs($5)
				Udiff = abs(Rupper-Eupper)
				
				if(Udiff <= 2)
					O1_upper++
				else if (Udiff <= Rupper)
					ON_upper++
				else if (Udiff <= Rupper*Rupper)
					ON2_upper++
				else
					OWorse_upper++				
				
				
				Rrange = Rupper+Rlower
				Erange = Eupper+Elower
				Rdiff = abs(Rrange-Erange)
				
				if(Rdiff <= 2)
					O1_range++
				else if (Rdiff <= Rrange)
					ON_range++
				else if (Rdiff <= Rrange*Rrange)
					ON2_range++
				else
					OWorse_range++	

				
			} 
			END {
				printf "Lower bound,%f,%f,%f,%f\n", 100*O1_lower/total, 100*ON_lower/total, 100*ON2_lower/total, 100*OWorse_lower/total; 
				printf "Upper bound,%f,%f,%f,%f\n", 100*O1_upper/total, 100*ON_upper/total, 100*ON2_upper/total, 100*OWorse_upper/total;
				printf       "Range,%f,%f,%f,%f\n", 100*O1_range/total, 100*ON_range/total, 100*ON2_range/total, 100*OWorse_range/total;
			}' $RAAnalysisSource > $RAChartSource

done

#Generate source files to plot another type of chart
for file in RAChartSource*.txt ; do

	RAChart=`echo $file | cut -d "." -f 2`
	awk -F, '$1 == "Lower bound" { printf "'$RAChart',%f,%f,%f,%f\n", $2, $3,  $4, $5 }' $file >> RAChartSource.LowerBound.txt
	awk -F, '$1 == "Upper bound" { printf "'$RAChart',%f,%f,%f,%f\n", $2, $3,  $4, $5 }' $file >> RAChartSource.UpperBound.txt
	awk -F, '$1 == "Range" { printf "'$RAChart',%f,%f,%f,%f\n", $2, $3,  $4, $5 }' $file >> RAChartSource.Range.txt

done

awk -F, 'BEGIN {print "Benchmark,1,n,n*n,imprecise"} $1 != "Benchmark" { print $0 }' RAChartSource.LowerBound.txt > LowerBound.csv
awk -F, 'BEGIN {print "Benchmark,1,n,n*n,imprecise"} $1 != "Benchmark" { print $0 }' RAChartSource.UpperBound.txt > UpperBound.csv
awk -F, 'BEGIN {print "Benchmark,1,n,n*n,imprecise"} $1 != "Benchmark" { print $0 }' RAChartSource.Range.txt > Range.csv

rm -f RAChart*.pdf

#Generate gnuplot charts
for file in RAChartSource*.txt ; do
	
	RAChart=`echo $file | sed -e "s/RAChartSource/RAChart/g" | sed -e "s/.txt/.pdf/g"`	
	bash GnuPlotScript.sh $file $RAChart

done

#Remove the temporary files
rm -f RARealValues*.txt
rm -f RAAnalysisSource*.txt
rm -f RAChartSource*.txt





