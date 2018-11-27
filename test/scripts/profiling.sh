#!/bin/bash

PIN=/home/1bp/Software/pin-3.5-97503-gac534ca30-gcc-linux/pin
ROOT=/home/1bp/rthms-dev
APPDIR=$ROOT/test/apps
TOOL=$ROOT/RTHMS

export OMP_NUM_THREADS=32

#Test Allocation with cache model
APP="$APPDIR/hooks/rthms_hooks"
$PIN -t $TOOL -o alloc_with_cache.out -k 1 -cache -- ${APP} > log_alloc_with_cache 2>&1


#Test Allocation without cache model
APP="$APPDIR/hooks/rthms_hooks"
$PIN -t $TOOL -o alloc_wo_cache.out -k 1 -- ${APP} > log_alloc_wo_cache 2>&1


#Test STREAM-Triad with cache model
APP="$APPDIR/stream/stream"
$PIN -t $TOOL -o triad_with_cache.out -k 1  -cache -- ${APP} > log_triad_with_cache 2>&1


#Test STREAM-Triad without cache model
APP="$APPDIR/stream/stream"
$PIN -t $TOOL -o triad_wo_cache.out -k 1 -- ${APP} > log_triad_wo_cache 2>&1
exit


