#!/bin/bash

PIN=/home/1bp/Software/pin-3.5-97503-gac534ca30-gcc-linux/pin
ROOT=/home/1bp/rthms-dev
APPDIR=$ROOT/test/apps
TOOL=$ROOT/RTHMS

export OMP_NUM_THREADS=32

#Test Hooks
APP="$APPDIR/hooks/rthms_hooks"
$PIN -t $TOOL -o hooks_with_cache.out -tracer -k 1 -cache -- ${APP} > log_hooks 2>&1


#Tracing STREAM-Triad
APP="$APPDIR/stream/stream"
$PIN -t $TOOL -o triad.out -tracer -k 1 -- ${APP} > log_triad 2>&1


#Tracing PENNANT
APP="$APPDIR/pennant/build/pennant $APPDIR/pennant/test/sedovsmall/sedovsmall.pnt"
#APP="$APPDIR/pennant/build/pennant $APPDIR/pennant/test/sedovflatx4/sedovflatx4.pnt"
$PIN -t $TOOL -o pennant.out -tracer -k 1 -- ${APP} > log_pennant 2>&1


#Tracing XSBench
APP="$APPDIR/XSBench/src/XSBench -t 32 -l 1000000 -s small"
#APP="$APPDIR/XSBench/src/XSBench -t 32 -l 1000000"
$PINDIR/pin -t $TOOL -o xs.out -tracer -k 1 -- ${APP} > log_XS 2>&1
exit

