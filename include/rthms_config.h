#ifndef RTHMS_CONFIG_H
#define RTHMS_CONFIG_H

#define HLINE "======================================================================"
#define CACHELINE_BITS  6 //64 byte cacheline
#define CACHELINE_BYTES 64UL
#define RTNSTACK_LVL 1
#define MAX_SRC_LEN  256
#define Mbyte        1048576

#define Threshold_LLCMBSize      0    //last-level cache size
#define Threshold_Size           1024   //ignore obj smaller than this size
#define Threshold_CPUIntensity   10000
#define Threshold_WriteIntensive 50
#define Threshold_Threads        2
#define Threshold_Significance   0.7
#define Threshold_Minor          0.001

#define ALLOCATED  999
#define UNFIT     -999
#define Beta      2
#define OUTPUT_FILENAME "pintrace"
#define USR_HOME_DIR "/home/"
#endif
