#!/bin/bash
#<< EOF

BarCount=`wc $1 -l | cut -d " " -f 1`

/usr/bin/gnuplot << EOC
set style data histograms
set style histogram rowstacked
set style fill solid 1.0 border -1

set boxwidth 0.5

set xrange [-1:$BarCount]

if ($BarCount > 3) set xtics rotate by -45; set xtic auto

set yrange [0:100]
set ylabel "% Variables"

set terminal postscript eps color
set output '| epstopdf --filter --outfile=$2' 

set datafile separator ","
set termoption enhanced

set style line 1 lt 1 lc rgb "black"
set style line 2 lt 1 lc rgb "gray" 
set style line 3 lt 1 lc rgb "white" 
set style line 4 lt 1 lc rgb "#696969" 

set key out horiz bot center Left reverse samplen 0.1

set grid ytics linestyle 1


plot '$1' using 2 ls 1 t "1", \
       '' using 3:xticlabels(1) ls 2 t "n", \
       '' using 4:xticlabels(1) ls 3 t "n^2", \
       '' using 5:xticlabels(1) ls 4 t "imprecise"
pause -1
EOC

