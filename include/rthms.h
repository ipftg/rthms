#ifndef RTHMS_H
#define RTHMS_H

#include <iostream>
#include <vector>
#include <string>
#include "rthms_config.h"

#define PANIC(format, ...){\
    printf(format "PANIC: %s %d\n", __VA_ARGS__, __FILE__, __LINE__);\
    exit(13);\
  }


#define ALERT(format, ...){\
    printf(format "ALERT: %s %d\n", __VA_ARGS__, __FILE__, __LINE__);\
  }


typedef UINT32 ID_T;

/****************************************/
/*     Signature of allocation/free functions     */
/****************************************/
class AllocFunc{
 public:
    std::string  name;
    int    argid_itemsize;
    int    argid_itemnum;
    int    argid_ptridx;
    bool   isInternal;
    AllocFunc(std::string& nm, int item_size, int num_item, int ptr_idx,bool i){
      name           = nm;
      argid_itemsize = item_size;
      argid_itemnum  = num_item;
      argid_ptridx   = ptr_idx;
      isInternal     = i;
    }
};

/****************************************/
/*     Specification of an access record*/
/****************************************/
typedef struct ACCESS_T{
  UINT64  timestamp;
  char    type;
  ADDRINT addr;
}ACCESS_T;


/****************************************/
/*     Specification of a Memory        */
/****************************************/
typedef struct Memory{
    ID_T    memory_id;
    std::string memory_name;
    double cost;
    double capacity_in_mb;
    double latency_read_ns;
    double latency_write_ns;
    double bandwidth_read_gbs;
    double bandwidth_write_gbs;
    double speedup_by_hwt;
    double speedup_by_lat;
    double speedup_by_bw;
    double locality_overhead_threshold;
}Memory;

extern bool compare_by_id_asc(const Memory& lhs, const Memory& rhs) ;
extern bool compare_by_cost_asc(const Memory& lhs, const Memory& rhs);
extern bool compare_by_readlat_asc(const Memory& lhs, const Memory& rhs);
extern bool compare_by_readbw_desc(const Memory& lhs, const Memory& rhs);
extern bool compare_by_writebw_desc(const Memory& lhs, const Memory& rhs) ;
extern bool compare_by_capacity_asc(const Memory& lhs, const Memory& rhs);


/****************************************/
/*     Statistics of Routine            */
/****************************************/
typedef struct RtnStats{
  UINT32 id;
  std::string name;
  bool entered;
  UINT64 visit;
  UINT64 memops; //accumulated
  UINT64 ins;
  UINT64 memops_snapshot;//during the current visit
  UINT64 ins_snapshot;
}RtnStats;


class GlobalMetrics{
public:
    UINT64 static_read_ins, static_write_ins;
    UINT64 dynamic_read_ins, dynamic_write_ins;
    UINT64 ins_cnt;
    std::string name_alloc_funcs;
    

    GlobalMetrics(){}
    GlobalMetrics(UINT64 static_read_,  UINT64 static_write_,
                  UINT64 dynamic_read_, UINT64 dynamic_write_,
                  UINT64 ins_cnt_, std::vector<AllocFunc> list_alloc_func_);
    
    void output(std::ostream &output=std::cout);
    
    UINT64 get_total_memref()const{
        return (dynamic_read_ins+dynamic_write_ins);
    }
};



typedef enum alloctype{
  STATIC_T=0,
  HEAP_T=1,
  STACK_T=2,
}AllocType;

const char AllocTypeStr[3][8]={"Static", "Heap", "Stack"};

class MetaObj;

/******************************************/
/*     Active Memory Object for tracing   */
/******************************************/
class TraceObj{
public:
    int     obj_id;
    ADDRINT st_addr;
    ADDRINT end_addr;
    UINT64  dynamic_read;
    UINT64  dynamic_write;
    UINT64  read_in_cache;
    UINT64  strided_read;
    UINT64  pointerchasing_read;
    UINT64  random_read;
    ADDRINT last_accessed_addr;
    ADDRINT last_accessed_addr_cacheline;
    ADDRINT last_accessed_addr_value;
    INT64   access_stride;
    time_t  start_time;
    time_t  end_time;
    UINT64 cnt;
    
    //constructor
    TraceObj(MetaObj& objref);
    
    void backup_trace(MetaObj* MetaObj_ptr, UINT64 ins_cnt, UINT64 memins_cnt);
    
    void record_read(ADDRINT addr, int granularity=0);
    void record_read(ADDRINT addr, bool isLLCHit);

    void record_write();
    
    void summary(std::ostream& output=std::cout);
    
    void print_short(std::ostream& output=std::cout);
};

class TraceObjMeta{
 public:
    int obj_id;
    ADDRINT st_addr;
    ADDRINT end_addr;

    TraceObjMeta(MetaObj& obj);
    void print_short(std::ostream& output=std::cout);
};


class TraceObjThreadAccess{
public:
    ADDRINT st_addr;
    UINT64  dynamic_read;
    UINT64  dynamic_write;
    UINT64  read_in_cache;
    UINT64  strided_read;
    UINT64  pointerchasing_read;
    UINT64  random_read;
    ADDRINT last_accessed_addr;
    ADDRINT last_accessed_addr_cacheline;
    ADDRINT last_accessed_addr_value;
    INT64   access_stride;
    time_t  start_time;
    
public:
    TraceObjThreadAccess(ADDRINT st_addr_, double time_){
        st_addr = st_addr_;
        start_time = time_;
        dynamic_read  = 0;
        dynamic_write = 0;
        read_in_cache = 0;
        strided_read  = 0;
        pointerchasing_read = 0;
        random_read  = 0;
        last_accessed_addr = 0;
        last_accessed_addr_cacheline = 0;
        last_accessed_addr_value = 0;
        access_stride = 0;
    }
    
    void record_read(ADDRINT addr, int granularity=0);
    void record_read(ADDRINT addr, bool isLLCHit);

    void record_write();
};


typedef struct ThreadMemAccess{
    int     thread_id;
    time_t  start_time;//when a thread start accessing
    time_t  end_time;  //when a thread is killed or the object is released
    
    ADDRINT accessed_addr_low;
    ADDRINT accessed_addr_high;
    UINT64  dynamic_read;
    UINT64  dynamic_write;
    UINT64  read_in_cache;
    UINT64  strided_read;
    UINT64  pointerchasing_read;
    UINT64  random_read;
}ThreadMemAccess;


/**************************************************/
/*     Memory Object allocated during execution   */
/**************************************************/

class  MetaObj{
 public:
    
    /*Identifier*/
    int     obj_id;
    ADDRINT st_addr;
    ADDRINT end_addr;
    ADDRINT size;
    int     creator_tid;
    ADDRINT ip;
    std::string  source_code_info;
    std::string  var_name;
    AllocType  type;
    double  start_time;
    double  end_time;
    UINT64  st_ins;
    UINT64  end_ins;
    UINT64  st_memins;
    UINT64  end_memins;
  
    /*Thread Accesses*/
    std::vector<ThreadMemAccess> access_list;
    
    /*Aggregated Raw Metrics from all threads*/
    UINT64  dynamic_read;
    UINT64  dynamic_write;
    UINT64  read_in_cache;
    UINT64  read_not_in_cache;
    UINT64  strided_read;
    UINT64  pointerchasing_read;
    UINT64  random_read;
    int     num_threads;

    /*Derived metrics from row metrics*/
    UINT64 mem_ref;
    double mem_ref_percentage;
    double size_in_mb;
    double readwrite_ratio;
    double readincache_ratio;
    double strided_read_ratio;
    double random_read_ratio;
    double pointchasing_read_ratio;
    double non_memory_ins_per_ref;
    
    /*Preference*/
    std::string preference_reason;
    double  *preference_order; //preference normalized by mem ref

    //constructor
    MetaObj();

    void reset();

    void release(UINT64 ins, UINT64 mem_ins, double t){
        end_ins = ins;
        end_memins = mem_ins;
        end_time = t;
    }

    void add_thread_accesses();
    void add_thread_accesses(int tid, TraceObjThreadAccess ta, double time_);

    void print_stats(std::ostream& output=std::cout);
    
    void print_meta(std::ostream& output=std::cout);

    void print_preference(std::ostream& output=std::cout);

    std::string getSourceCodeInfo(){return source_code_info;}

    bool operator<(const MetaObj& rhs) const
    {
    return (dynamic_read+dynamic_write) > (rhs.dynamic_read + rhs.dynamic_write);
    }

};




#endif
