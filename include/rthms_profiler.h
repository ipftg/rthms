#ifndef RTHMS_PROFILER_H
#define RTHMS_PROFILER_H

#ifdef TIMING
#include <time.h>
#include <sys/time.h>
#endif

#include <map>
#include <vector>
#include <sys/syscall.h>
#include <iomanip>
#include "rthms.h"
#include "rthms_config.h"
#include "rthms_utility.h"
#include "rthms_elf.h"
#include "rthms_analyzer.h"
//#include "rthms_allocator.h"
#include "pin.H"


using namespace std;

typedef enum{
    FUNC_OPEN,
    FUNC_CLOSE
}FuncEvent;



/* ===================================================================== */
/*Thread-local Data Structure stored in TLS*/
/* ===================================================================== */

typedef struct ThreadDataType
{
  // the memory object allocated by the give tracked alloc func
  MetaObj curr_allocobj; 
  
  // intenal alloc func if required
  vector<MetaObj> open_alloc_list;
  
  map<int,TraceObjThreadAccess> accessed_objects;

  uint64_t ins;
  FILE*  tfile;
  ACCESS_T*  buf;
  size_t buf_pos;
  size_t total;
}ThreadDataType;

/* function to access thread-specific data*/
ThreadDataType* get_tls(TLS_KEY tls_key, THREADID threadid)
{
    ThreadDataType* tdata = static_cast<ThreadDataType*>(PIN_GetThreadData(tls_key, threadid));
    return tdata;
}

/* ===================================================================== */
// Call path related
/* ===================================================================== */

typedef struct OpenAllocObj{
    int  obj_id;
    char rtn_stack[RTNSTACK_LVL][MAX_SRC_LEN];
    int  timestamp;
    int  rtn_level;
}OpenAllocObj;


double gettime_in_sec(){
    double time_sec=0.0;
    struct timeval time_st;
    if (gettimeofday(&time_st,NULL)){
        cerr << "unable to gettime, skip timing\n";
        time_sec=-1.;
    }else{
        time_sec = (double)time_st.tv_sec + (double)time_st.tv_usec * .000001;
    }
    return time_sec;
}


bool compare_by_memintensity(const RtnStats& lhs, const RtnStats& rhs);

void track_callpath(VOID *rtnAddr, FuncEvent eventType,ADDRINT  returnIp,const string* rtn_name,THREADID tid);

void print_metaobj(vector<MetaObj>& masterlist, ostream& output=std::cout);

void upd_static_allocation(vector<MetaObj>& masterlist);

#endif
