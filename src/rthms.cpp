#include "rthms.h"
#include <iomanip>

using namespace std;

GlobalMetrics::GlobalMetrics(UINT64 static_read_,  UINT64 static_write_,
                             UINT64 dynamic_read_, UINT64 dynamic_write_,
                             UINT64 ins_cnt_, vector<AllocFunc> list_alloc_func_)
{
    
    static_read_ins  = static_read_;
    static_write_ins = static_write_;
    dynamic_read_ins = dynamic_read_;
    dynamic_write_ins= dynamic_write_;
    ins_cnt = ins_cnt_;
    
    vector<AllocFunc>::iterator it;
    for(it=list_alloc_func_.begin(); it<list_alloc_func_.end();++it){
        name_alloc_funcs.append(it->name);
        name_alloc_funcs.append("\n");
    }
}


void GlobalMetrics::output(ostream &output)
{
    output <<HLINE<<endl;
    output <<"Summary:"<<endl;
    output <<HLINE<<endl;
    
    output <<right<<setw(24)<<"Traced Allocation Rtn: "<<setw(31)<<name_alloc_funcs<<endl;
    output <<right<<setw(24)<<"Static  Read Ins: "<<setw(15)<< static_read_ins  <<setw(16)<<"Write Ins: "     <<setw(15)<<static_write_ins <<endl;
    output <<right<<setw(24)<<"Dynamic Read Ins: "<<setw(15)<< dynamic_read_ins <<setw(16)<<"Write Ins:"      <<setw(15)<<dynamic_write_ins<<endl;
}


//constructor
MetaObj::MetaObj(){ MetaObj::reset();}

void MetaObj::reset(){
  
  /*Identifier*/
    obj_id = -1;
    st_addr= 0;
    end_addr= 0;
    size= 0;
    ip  = 0;
    creator_tid = -1;
    start_time= 0.;
    end_time= 0.;
    st_ins= 0 ;
    end_ins= 0;
    st_memins= 0;
    end_memins= 0;
    source_code_info.clear();
    var_name.clear();

  /*Aggregated Raw Metrics from all threads*/
    dynamic_read = 0;
    dynamic_write = 0;
    read_in_cache = 0;
    read_not_in_cache = 0;
    strided_read = 0;
    pointerchasing_read = 0;
    random_read = 0;
    num_threads = 0;

  /*Derived metrics from row metrics*/
    mem_ref = 0;
    mem_ref_percentage = 0.;
    size_in_mb = 0.;
    readwrite_ratio = 0.;
    readincache_ratio = 0.;
    strided_read_ratio = 0.;
    random_read_ratio = 0.;
    pointchasing_read_ratio = 0.;
    non_memory_ins_per_ref = 0.;  
}
  

void MetaObj::print_stats(ostream& output){

  output<<left
	<< setw(6)<<obj_id
	<< setw(6)<<num_threads
	<< setw(8)<<fixed<<setprecision(1)<< size_in_mb
        << right
    // << setw(6)<<fixed<<setprecision(1)<< (st_ins* 1.0e-9) 
    // << setw(6)<<fixed<<setprecision(1)<< (end_ins*1.0e-9)
	<< setw(10)<<fixed<<setprecision(1)<< (mem_ref*1.0e-6>1.0 ?(mem_ref*1.0e-6) : mem_ref)
	<<(mem_ref*1.0e-6>1.0 ?"mil" :"")
	<< setw(10)<<fixed<<setprecision(1)<< mem_ref_percentage<<"%"
    // << setw(6)<<fixed<<setprecision(1)<< non_memory_ins_per_ref
	<< setw(10)<<fixed<<setprecision(1)<< (dynamic_read*1.0e-6>1.0   ?(dynamic_read*1.0e-6): dynamic_read)
	<<(dynamic_read*1.0e-6 > 1.0 ?"mil" : "")
	<< setw(10)<<fixed<<setprecision(0)<< (dynamic_write*1.0e-6>1.0  ?(dynamic_write*1.0e-6):dynamic_write)
	<< (dynamic_write*1.0e-6 > 1.0 ?"mil" : "")
	<< setprecision(1)<<setw(6)<< readwrite_ratio<<"%"
	<< setprecision(1)<<setw(6)<< readincache_ratio<<"%"
	<< read_not_in_cache
	<< setprecision(1)<<setw(6)<< strided_read_ratio<<"%"
	<< setprecision(1)<<setw(6)<< random_read_ratio<<"%"
	<< setprecision(1)<<setw(6)<< pointchasing_read_ratio<<"%"
	<<endl;
}

void MetaObj::print_meta(ostream& output){
  
  output << "\nObj Id        : " << obj_id   << "\n"
	 << "Type          : " << AllocTypeStr[type] << "\n"
	 << "Var Name      : " << var_name.c_str() << "\n"
         << "Location      : " << source_code_info << "\n"
	 << "Creator Thread: " << creator_tid << "\n"
	 << "Address Space : ["<< st_addr << " - " << end_addr <<"]\n"
         << "Size in Byte  : " << size;
}


void MetaObj::add_thread_accesses(int tid, TraceObjThreadAccess ta, double time_){
    ThreadMemAccess a;
    a.thread_id     = tid;
    a.start_time    = ta.start_time;
    a.end_time      = time_;
    a.dynamic_read  = ta.dynamic_read;
    a.dynamic_write = ta.dynamic_write;
    a.read_in_cache = ta.read_in_cache;
    a.strided_read  = ta.strided_read;
    a.pointerchasing_read=ta.pointerchasing_read;
    a.random_read   = ta.random_read;
    access_list.push_back(a);
}

//constructor
TraceObj::TraceObj(MetaObj& objref){
    //MetaObj_ptr = &objref;
	obj_id  = objref.obj_id;
        st_addr = objref.st_addr;
        end_addr= objref.end_addr;
        dynamic_read  = 0;
        dynamic_write = 0;
	cnt=0;
	read_in_cache = 0;
        strided_read  = 0;
        pointerchasing_read = 0;
        random_read         = 0;
        last_accessed_addr  = 0;
	last_accessed_addr_cacheline=0;
        last_accessed_addr_value = 0;
        access_stride = 0;
}
    
void TraceObj::backup_trace(MetaObj* MetaObj_ptr, UINT64 ins_cnt, UINT64 memins_cnt){
      MetaObj_ptr->end_ins       = ins_cnt;//capture the curr global instruction count
      MetaObj_ptr->end_memins    = memins_cnt;
      MetaObj_ptr->dynamic_read  = dynamic_read;
      MetaObj_ptr->dynamic_write = dynamic_write;
      MetaObj_ptr->read_in_cache = read_in_cache;
      MetaObj_ptr->strided_read  = strided_read;
      MetaObj_ptr->pointerchasing_read = pointerchasing_read;
      MetaObj_ptr->random_read   = random_read; 
      //MetaObj_ptr->printout();
      //cout<< "backup active obj"<<objId<<", MetaObj_ptr="<<MetaObj_ptr<<", dynamic_read "<<dynamic_read <<", dynamic_write "<<dynamic_write<<endl;
      //cout<< "master obj"<<MetaObj_ptr->objId<<", dynamic_read "<<MetaObj_ptr->dynamic_read <<", dynamic_write "<<MetaObj_ptr->dynamic_write<<endl;
      //MetaObj_ptr->summary();
}

void TraceObj::summary(ostream& output){

  output << "Trace Obj Id "<<right << setw(8) <<  obj_id
       << setw(10)<<fixed<<setprecision(1)<< dynamic_read*1.0e-6
       << setw(10)<<fixed<<setprecision(1)<< dynamic_write*1.0e-6
       << setw(10)<<fixed<<setprecision(1)<< read_in_cache*1.0e-6
       << setw(10)<<fixed<<setprecision(1)<< strided_read*1.0e-6
       << setw(10)<<fixed<<setprecision(1)<< pointerchasing_read*1.0e-6
       << setw(10)<<fixed<<setprecision(1)<< random_read*1.0e-6
       <<endl;
}


void TraceObj::print_short(ostream& output){
    
    output << "Trace Obj Id " << obj_id 
	   << ", addr ["<< st_addr << ", " << end_addr <<"]" << endl;
    
}
/*
void TraceObjThreadAccess::record_read(ADDRINT addr, int granularity){
    
    dynamic_read ++;
    
    //filter out the same cache line
    ADDRINT cacheline = (addr-st_addr)>>CACHELINE_BITS;
    if(cacheline == last_accessed_addr_cacheline){
        read_in_cache ++;
        return;
    }
    
    //match exact memory address
    if(granularity==0){
        if( (last_accessed_addr+access_stride) == addr)
            strided_read ++;
        else if( addr == last_accessed_addr_value)
            pointerchasing_read ++;
        else
            random_read ++;
        
        access_stride = addr - last_accessed_addr;
        last_accessed_addr = addr;
        last_accessed_addr_cacheline = cacheline;
        
        ADDRINT* p = (ADDRINT*)addr;
        last_accessed_addr_value = *p;        
    }
    else{//match exact cacheline
        if( (last_accessed_addr_cacheline+access_stride) == cacheline)
            strided_read ++;
        else if(addr == last_accessed_addr_value)
            pointerchasing_read ++;
        else
            random_read ++;
        
        access_stride = cacheline-last_accessed_addr_cacheline;
        last_accessed_addr = addr;
        last_accessed_addr_cacheline = cacheline;
        
        ADDRINT* p = (ADDRINT*)addr;
        last_accessed_addr_value = *p;
    }
    }*/


void TraceObjThreadAccess::record_read(ADDRINT addr_in, bool isLLCHit){
    
    dynamic_read ++;
    
    if(isLLCHit){
        read_in_cache ++;
	return;
    }
    
    //stride in the granularity of cacheline
    //allow negative stride
    ADDRINT  addr = (addr_in >>CACHELINE_BITS);
    addr = addr<<CACHELINE_BITS;
    if( ((INT64)last_accessed_addr+access_stride) == (INT64)addr)
      strided_read ++;
    else{ 
      access_stride = (INT64)addr - (INT64)last_accessed_addr;
      if(addr_in == last_accessed_addr_value)
	pointerchasing_read ++;
      else
	random_read ++;
    }

    ADDRINT* p = (ADDRINT*)addr_in;
    last_accessed_addr_value = *p;
    last_accessed_addr = addr;

    //cout << "addr_in "<<std::hex<<addr_in<< " addr "<<addr<<" strided_read "<<strided_read << " random_read "<<random_read <<endl;
}

void TraceObjThreadAccess::record_write(){
    dynamic_write ++;
}


TraceObjMeta::TraceObjMeta(MetaObj& obj){
  obj_id = obj.obj_id;
  st_addr= obj.st_addr;
  end_addr=obj.end_addr;
}


void TraceObjMeta::print_short(ostream& output){

  output << "Trace Obj Id " << obj_id
	 << ", addr ["<< st_addr << ", " << end_addr <<"]" << endl;

}
