#!/bin/bash

hpcrun='/home/1bp/Software/hpctoolkit/build/bin/hpcrun'
hpcstruct='/home/1bp/Software/hpctoolkit/build/bin/hpcstruct'
hpcprof='/home/1bp/Software/hpctoolkit/build/bin/hpcprof'

export OMP_NUM_THREADS=32

#$hpcrun --event PAPI_TOT_CYC --event PAPI_TOT_INS --event PAPI_L3_TCM ./build/pennant test/sedovflatx4/sedovflatx4.pnt > output2 2>&1
#$hpcstruct ./build/pennant
#$hpcprof -S pennant.hpcstruct -I /home/1bp/ApplicationPool/PENNANT/src/'*' hpctoolkit-pennant-measurements

./build/pennant ./test/sedovflatx4/sedovflatx4.pnt


