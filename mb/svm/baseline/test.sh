#!/bin/bash
BMDIR="$(dirname "${BASH_SOURCE[0]}")"
MEASURE="$BMDIR/../../common/measure.py"
 #ls -l  data/source/mb/svm/ 
export OMP_NUM_THREADS=4
$MEASURE ./bin/thundersvm-train -t 0 data/source/mb/svm/train.dat data/source/mb/svm/model.dat
$MEASURE ./bin/thundersvm-train -t 0 data/source/mb/svm/phishing  data/source/mb/svm/model2.dat
$MEASURE ./bin/thundersvm-train -t 0 data/source/mb/svm/news20.binary  data/source/mb/svm/model3.dat
$MEASURE ./bin/thundersvm-train -t 0 data/source/mb/svm/real-sim data/source/mb/svm/model4.dat
$MEASURE ./bin/thundersvm-train -t 0 data/source/mb/svm/w8a data/source/mb/svm/model5.dat
