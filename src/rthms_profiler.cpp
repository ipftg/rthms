#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rthms_hooks.h"
#include "rthms_profiler.h"
#include "rthms_cache.h"

#include <execinfo.h>
#include <signal.h>


using namespace std;

#ifndef BUF_SIZE
#define BUF_SIZE 81920
#endif

/* ===================================================================== */
// Command line
/* ===================================================================== */
//get output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", 
			    "o", "", 
			    "specify output file name");

//get the configuration of memories
KNOB<string> KnobMemConfigFile(KNOB_MODE_WRITEONCE, "pintool",
                            "m", "",
                            "specify the available memories");

//Option I: get user-specified functions to track
KNOB<string> KnobTrackFuncFile(KNOB_MODE_WRITEONCE, "pintool",
			       "t", "",
			       "specify functions to track");

//Option II: get hooks to track a specific scope
KNOB<int> KnobHook(KNOB_MODE_WRITEONCE, "pintool",
		   "k", "0","specify how many times of tracking the hook");

//get user-specified allocation rtn
KNOB<string> KnobAllocFuncFile(KNOB_MODE_WRITEONCE, "pintool",
			       "a", "",
			       "specify allocation routines to track");

//get user-specified free rtn
KNOB<string> KnobFreeFuncFile(KNOB_MODE_WRITEONCE, "pintool",
			      "f", "",
			      "specify free routines to track");

//get option to trace call path 
KNOB<bool> KnobTrackCallPath(KNOB_MODE_WRITEONCE, "pintool",
			     "callpath", "false", "Include call path to malloc function."
			     "Default is off. ");

//get option to enable cache model
KNOB<bool> KnobCache(KNOB_MODE_WRITEONCE, "pintool",
		     "cache", "false", "Enable a basic cache model."
		     "Default is off. ");

//get option to use the tool as a tracer or profiler
KNOB<bool> KnobTracer(KNOB_MODE_WRITEONCE, "pintool",
			   "tracer", "false", "Use the tool as a tracer instead of a profiler."
			   "Default is off. ");

//get option to enable memory trace output
KNOB<bool> KnobOutputMemTrace(KNOB_MODE_WRITEONCE, "pintool",
			      "memtrace", "false", "Enable memory trace output."
			      "Default is off. ");


/* =========== */
/* Usage       */
/* =========== */

INT32 Usage()
{
  cerr << "A Pintool collects array-centric metrics." << endl;
  cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
  return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
vector<string>    list_track_func;
vector<string>    list_hooks;
vector<AllocFunc> list_alloc_func;
vector<AllocFunc> list_free_func;
vector<Memory>    memory_list;
vector<OPCODE>    op_list;
size_t hook_num;

UINT64 global_ins_cnt, rtn_ins_cnt;
UINT64 dynamic_read_ins, dynamic_write_ins;
UINT64 static_read_ins,static_write_ins; 

ADDRINT img_addr_low,img_addr_high;
ADDRINT stack_bound_low, stack_bound_high;
bool is_record_img,is_record_rtn,is_track_call,hasCalledFini;
map<UINT32, RtnStats> rtnstats_map;
map<string, string> var_map;

vector<MetaObj> masterlist;
vector<TraceObjMeta> activelist;
ofstream output_file;
//FILE* memtrace_file;
string memtrace_file_name;

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v);
VOID Fini(INT32 code, VOID *v);

L3::CACHE *l3;
bool isL3WriteThrough, isL3WriteAllocate;
double time_start;


/* ===================================================================== */
// Thread related
/* ===================================================================== */
map<THREADID,int> thread_map;
// key for accessing TLS storage in the threads
static  TLS_KEY tls_key;
PIN_LOCK lock;


/* ===================================================================== */
// Call path related
/* ===================================================================== */
vector<OpenAllocObj> open_allocobj_list;

/* ===================================================================== */
// Analysis Functions
/* ===================================================================== */
VOID TraceMem(ADDRINT addr, INT32 size, BOOL isRead,  THREADID tid)
{    
  if(!is_record_img || !is_record_rtn) return;

  if(isRead)
        dynamic_read_ins ++;
  else
        dynamic_write_ins ++;

  /* currently, stack read/write is filtered
    if(stack_bound_low< addr && addr < stack_bound_high){
        if(isRead)
            dynamic_stackread_ins ++;
        else
            dynamic_stackwrite_ins ++;
        return;
    }
  */

  ThreadDataType* t_data = get_tls(tls_key, tid);

  //Start of Cache
  CACHE_SET::CACHEBLOCK evict_block;
  memset(&evict_block,0,sizeof(CACHE_SET::CACHEBLOCK));
  if(KnobCache){
    CACHE_BASE::ACCESS_TYPE accessType = (isRead ?CACHE_BASE::ACCESS_TYPE_LOAD : CACHE_BASE::ACCESS_TYPE_STORE);

    PIN_GetLock(&lock, tid+1);
    evict_block = l3->lookup_evict_update(addr,accessType);
    PIN_ReleaseLock(&lock);   
  }
  //End of Cache 

  //for profiler, capture address and object name
  if(!KnobTracer){

    //int evict_objid = -1;
    int id=-1;
    vector<TraceObjMeta>::iterator it;
    for (it=activelist.begin();it!=activelist.end();it++){
      //if( it->st_addr<= evict_block.addr && evict_block.addr<= it->end_addr)
      //evict_objid = it->obj_id;
      
      if( it->st_addr<= addr && addr <= it->end_addr){
	id = it->obj_id;
	break;
      }
    }

    //check and update thread-local accessed objects
    if(id>=0){
      map<int, TraceObjThreadAccess>::iterator find = t_data->accessed_objects.find(id);
      if( find!= (t_data->accessed_objects).end()){
	    if(isRead)
	      (find->second).record_read(addr, (evict_block.tag==1));
	    else
	      (find->second).record_write();
      }else{
	    TraceObjThreadAccess newaccess(id, tid);
	    if(isRead)
	      newaccess.record_read(addr,  (evict_block.tag==1));
	    else
	      newaccess.record_write();
	    (t_data->accessed_objects).insert(make_pair<int, TraceObjThreadAccess>(id, newaccess));
      }
      
      //cout << "obj"<<id<<" addr "<<std::hex<<addr<<std::dec << " hit "<<ul3Hit<<endl;
    }

  }//End of Profiler

  //Start of saving traces
  if(KnobTracer || KnobOutputMemTrace ){

    //Mode I: dram_trace
#ifndef CPU_TRACE 

    //read miss (incl. I->M)
    if( isRead && (evict_block.tag!=1 || evict_block.valid!=1)){ 
      //ACCESS_T r = {t_data->ins, 'R', addr};
      ACCESS_T r = {global_ins_cnt, 'R', addr};
      t_data->buf[t_data->buf_pos++]=r;
    }
    //write through 
    else if(!isRead && isL3WriteThrough){
      //ACCESS_T r = {t_data->ins, 'W', addr};
      ACCESS_T r = {global_ins_cnt, 'W', addr};
      t_data->buf[t_data->buf_pos++]=r;
    }
    //Read,Miss,(invalid) or Write,WriteThrough or Write,Writeback,Miss                                      
    else if((isRead || isL3WriteAllocate) && (evict_block.tag==0 && evict_block.dirty==1)){
      //ACCESS_T r = {t_data->ins, 'W', evict_block.addr};
      ACCESS_T r = {global_ins_cnt, 'W', evict_block.addr};
      t_data->buf[t_data->buf_pos++]=r;
    }

    //Mode II: cpu_trace 
#else
    if(isRead && (evict_block.tag==0 || evict_block.valid==0))
    {
      if(evict_block.tag==0 && evict_block.dirty==1 ){
	fprintf(t_data->tfile, "%lu %lu %lu\n", t_data->ins, addr, evict_block.addr);
      }
      else{
	fprintf(t_data->tfile, "%lu %lu\n", t_data->ins, addr);
      }

      t_data->ins = 0;
    }
#endif

    //flush buffer
    if(t_data->buf_pos==BUF_SIZE){
      //cout << "flush trace t"<<tid<<endl;
      fwrite(t_data->buf, sizeof(ACCESS_T),BUF_SIZE , t_data->tfile);
      t_data->buf_pos=0;
      t_data->total +=BUF_SIZE;
    }

  }//End of saving traces

}


VOID AllocBefore(ADDRINT itemsize, ADDRINT num_item, ADDRINT returnip,
		 THREADID tid,  UINT32 fid)
{
#ifdef DEBUG
  cout << "Enter AllocBefore: fid "<< fid
       << ", returnip " <<returnip << endl;
#endif

    if(!is_record_img) return;
    //if(returnip<img_addr_low || returnip>img_addr_high ) return;

    size_t size = itemsize*num_item;    
    if(size<Threshold_Size) return;


    ThreadDataType* t_data = get_tls(tls_key, tid);

    //Always overwrite if External
    if( !list_alloc_func[fid].isInternal ){
      (t_data->curr_allocobj).reset();
      (t_data->curr_allocobj).ip   = returnip;
      (t_data->curr_allocobj).size = size;
      (t_data->curr_allocobj).creator_tid = tid;

    }else{

      //return if no External Alloc being tracked
      if((t_data->curr_allocobj).creator_tid == -1) return;

      if( (t_data->open_alloc_list).size()==0 ||
	  (t_data->open_alloc_list).back().ip!=returnip)
	{
	  MetaObj anew;
	  (t_data->open_alloc_list).push_back(anew);	  
	}

      //update to the threads private record
      (t_data->open_alloc_list).back().ip   = returnip;
      (t_data->open_alloc_list).back().size = size;
    }
#ifdef DEBUG
  cout << "Exit AllocBefore: " << list_alloc_func[fid].name 
       << ", returnip " <<returnip << endl;
#endif

}


VOID AllocAfter(ADDRINT ret, ADDRINT returnip,THREADID tid, 
		UINT32 fid)
{  
#ifdef DEBUG
  cout << "Enter AllocAfter: fid "<< fid
       << ", returnip " <<returnip << endl;
#endif
  
    if(!is_record_img) return;
    //if(returnip<img_addr_low || returnip>img_addr_high ) return;
        
    ThreadDataType* t_data = get_tls(tls_key, tid);
    MetaObj&  curr_memobj  = t_data->curr_allocobj;

    //return if no External Alloc being tracked
    if(curr_memobj.creator_tid < 0) return;

    //return if an External Alloc cannot find matched open
    bool isExternal = (!list_alloc_func[fid].isInternal);
    if( isExternal && curr_memobj.ip!=returnip) return;

    //avoid adding object with duplicate address space
    vector<TraceObjMeta>::iterator it;
    for(it=activelist.begin(); it!=activelist.end();++it){
      if( it->st_addr==ret){
	ALERT("The address %lx has already been allocated to %s %s",
	      ret, masterlist[it->obj_id].source_code_info.c_str(), 
	      masterlist[it->obj_id].var_name.c_str());

	//It could be realloc : make it free first and then add the new address
	masterlist[it->obj_id].release(global_ins_cnt,(dynamic_read_ins+dynamic_write_ins), gettime_in_sec());
	activelist.erase(it);
	break;
      }
    }

    /**
     * Start: get source code info
     */
    string var_str;
    string curr_filename;
    INT32  curr_line;
    INT32  curr_column;
    PIN_LockClient();
    PIN_GetSourceLocation(returnip, &curr_column, &curr_line, &curr_filename);
    PIN_UnlockClient();

    stringstream ss;
    if(curr_filename.length()>0 && curr_filename.find_last_of("/")!=string::npos){
      ss << curr_filename << ":"<< list_alloc_func[fid].name << ":"<< curr_line;
    }
#ifdef DEBUG
    else{
      cout<< "Source information unavailable:"<< list_alloc_func[fid].name<<endl;
    }
#endif

    //search for variable name if isExternal
    if(isExternal && curr_filename.length()>0){
  
      //check if this var_name has been captured before
      string key = ss.str();
      map<string, string>::iterator vit = var_map.find(key);
      if(vit==var_map.end()){
	std::ifstream file(curr_filename.c_str());

	if( file.is_open() )
	{
	  file.seekg(ios::beg);
	  string ll;
	  int extend_search_line = 2;
	  int lnum=0;
	  while(lnum ++ < curr_line) getline(file, ll);

	  while((ll.length()==0 || ll.find_first_of(list_alloc_func[fid].name)==string::npos) 
		&& (lnum ++ < (curr_line+extend_search_line))) 
	    getline(file, ll); //allow search one line before and after 

	  lnum=curr_line-extend_search_line;
	  while((ll.length()==0 || ll.find_first_of(list_alloc_func[fid].name)==string::npos) 
		&& (lnum ++ <curr_line)) 
	    getline(file, ll); //allow search one line before and after 
		
	  if(ll.length()>0){

	    size_t head = ll.find_first_not_of(" \t\n\v\f\r");
	    size_t tail = ll.find_last_not_of(" \t\n\v\f\r");
	    
	    if(list_alloc_func[fid].argid_ptridx == -1){
	      tail =  ll.find_first_of("=");
	    }
#ifdef DEBUG
	    cout << "Enter AllocAfter: " << fid
		 << ", returnip " <<returnip <<" "<<ss.str()<<" ll" <<ll<<" head "<<head <<" tail"<<tail << endl;
#endif
	    ll = ll.substr(head, tail-head);
	    var_map.insert(make_pair(key, ll));
	    var_str = ll;
#ifdef DEBUG
	    cout << "Insert to var_map "<< key << ":" << var_str<<endl;
#endif
	  }
	}else{
#ifdef DEBUG
	  if(curr_filename.length()>0)
	    cout << "Unable to open the source code file "<< curr_filename 
		 << ", "<<list_alloc_func[fid].name<<endl;
#endif

	  //either skip this object or start tracking call path	  
	  if(KnobTrackCallPath){
	    isExternal = false;
	    is_track_call = true;

	    //reset tracking
	    (t_data->open_alloc_list).clear();
	    curr_memobj.st_addr  = ret;
	    curr_memobj.end_addr = curr_memobj.st_addr + curr_memobj.size - 1;
	    curr_memobj.creator_tid = tid;
	  }else return;

	}
      }else{
	  var_str = vit->second;
      }
    }
    /**
     * End: get source code info
     */


    /**
     * Start: add the data object
     */
    if( isExternal ){//add the new object

      curr_memobj.var_name = var_str;
      curr_memobj.source_code_info = ss.str();
      curr_memobj.type     = HEAP_T;
      curr_memobj.obj_id   = masterlist.size();
      curr_memobj.st_addr  = ret;
      curr_memobj.end_addr = curr_memobj.st_addr + curr_memobj.size - 1;
      curr_memobj.creator_tid = tid;
      curr_memobj.start_time  = gettime_in_sec();
      curr_memobj.st_ins      = global_ins_cnt;
      curr_memobj.st_memins   = dynamic_read_ins+dynamic_write_ins;

      //add additional info from internal routines
      size_t len = (t_data->open_alloc_list).size();
      for(size_t j=0;j<len;j++)
	{
	  if((t_data->open_alloc_list[j]).source_code_info.length()>0 ){

	    if( (t_data->open_alloc_list[j]).st_addr != ret){
	      cerr << (t_data->open_alloc_list[j]).source_code_info<< ": " 
		   << (t_data->open_alloc_list[j]).st_addr <<" != "<<ret<<endl;

	    }else{
	      curr_memobj.source_code_info.append(" -> ");
	      curr_memobj.source_code_info.append((t_data->open_alloc_list[j]).source_code_info);
	      if((t_data->open_alloc_list[j]).size > curr_memobj.size)
		{
		  curr_memobj.size = (t_data->open_alloc_list[j]).size;
		  curr_memobj.end_addr = curr_memobj.st_addr + curr_memobj.size - 1;
		}
	    }
	  }
	}


      if(KnobTrackCallPath){
     	is_track_call = true;
      }else{
	//only track those with src info
	if(curr_memobj.source_code_info.find_first_not_of(" \t\n\v\f\r") != string::npos)
	{
	  //Update Shared Data structures
	  PIN_GetLock(&lock, tid+1);
	  masterlist.push_back(curr_memobj);

	  //add to active list
	  MetaObj& a = masterlist.back();
	  TraceObjMeta b( a );
	  activelist.push_back(b);
	  PIN_ReleaseLock(&lock);
#ifdef VERBOSE
	  a.print_meta();
#endif
	}

	//reset tracking
	curr_memobj.reset();
	(t_data->open_alloc_list).clear();
      }

    }else{//Internal 
      size_t len = (t_data->open_alloc_list).size();
      for(int j=len-1;j>=0;j--)
	{
	  if((t_data->open_alloc_list[j]).ip == returnip){
	    (t_data->open_alloc_list[j]).source_code_info=ss.str();
	    (t_data->open_alloc_list[j]).st_addr = ret;
	    break;
	  }
	}
    }

#ifdef DEBUG
    cout << "\nExit AllocAfter "<<endl;
#endif

}


VOID FreeBefore(ADDRINT addr,ADDRINT returnIp, THREADID tid)
{
    if(!is_record_img) return;

    PIN_GetLock(&lock, tid+1);
    vector<TraceObjMeta>::iterator it;
    for(it=activelist.begin();it!=activelist.end(); ++it){
        if( it->st_addr==addr ){
            masterlist[it->obj_id].release(global_ins_cnt,
					   (dynamic_read_ins+dynamic_write_ins), 
					   gettime_in_sec());
            activelist.erase(it);
            break;
        }
    }
    PIN_ReleaseLock(&lock);
    
}


VOID start_trace_main(VOID* rtnptr, ADDRINT returnIp)
{

  cout << "start tracing main "<< is_record_rtn <<endl;
  
  //switch on tracing only after entering the main rtn
  is_record_img = true;

  if(KnobTrackCallPath) {
    track_callpath(rtnptr, FUNC_OPEN, returnIp,new string("main"),0);
  }
}

VOID end_trace_main(VOID* rtnptr, ADDRINT returnIp)
{
  cout << "end tracing main" <<endl;

  if(KnobTrackCallPath) {
    track_callpath(rtnptr, FUNC_CLOSE, returnIp,new string("main"),0);
  }
  //switch off tracing
  is_record_img = false;
}

VOID DetachFunc(VOID *v)
{ 

  Fini(0,NULL);
}

VOID HookFunc(THREADID tid, ADDRINT op) {

  map<UINT32, RtnStats>::iterator it;

  if(op==OP_HOOK_OPEN){

    it = rtnstats_map.find(op);
    if(it!= rtnstats_map.end() && (it->second).entered)
      PANIC("recursively entering a HOOK [tid = %i]", tid);

    is_record_rtn = true;

    if(it!= rtnstats_map.end()){
      (it->second).entered = true;
      (it->second).visit  ++;
      (it->second).memops_snapshot = (dynamic_read_ins+dynamic_write_ins);
      (it->second).ins_snapshot = rtn_ins_cnt;
    }else{
      RtnStats r;
      r.id = op;
      r.name = "HOOK_OPEN";
      r.memops = 0;
      r.ins = 0;
      r.entered = true;
      r.visit = 1;
      r.memops_snapshot = (dynamic_read_ins+dynamic_write_ins);
      r.ins_snapshot = rtn_ins_cnt;
      rtnstats_map.insert(make_pair(op, r));
    }

#ifdef VERBOSE
    cout << "HOOK Open: "<<(it->second).visit<< "/"<<hook_num<< ", snapshot "<< rtn_ins_cnt <<endl;
#endif

  }else if (op==OP_HOOK_CLOSE){

    it = rtnstats_map.find(OP_HOOK_OPEN);
    if(it== rtnstats_map.end()  || !(it->second).entered )
      PANIC("Never entering a HOOK [tid = %i]", tid);

    (it->second).entered = false;
    (it->second).memops  += ((dynamic_read_ins+dynamic_write_ins) - (it->second).memops_snapshot);
    (it->second).ins     += (rtn_ins_cnt - (it->second).ins_snapshot);

#ifdef VERBOSE
    cout << "HOOK Close: "<<(it->second).visit<< "/"<<hook_num<< ", snapshot "<< rtn_ins_cnt <<endl;
#endif

    if((it->second).visit==hook_num){
#ifdef VERBOSE
      cout << "Scheduled to detach. "<<rtn_ins_cnt<< endl;
#endif
      PIN_GetLock(&lock,1);
      is_record_img = false;
      is_record_rtn = false;
      PIN_ReleaseLock(&lock);
      PIN_Detach();//the actual occuring point is controlled by pin
    }

    //need to check all track func is off
    is_record_rtn = false;    
    for(it=rtnstats_map.begin();it!=rtnstats_map.end();it++){
      if( (it->second).entered ){
	is_record_rtn = true;
	break;
      }
    }
  }

  return;
}

VOID start_track_rtn(const string* rtn, UINT32 rid) {
#ifdef VERBOSE
  cout << "start tracing rtn "<< *rtn<<endl;
#endif

    is_record_rtn = true;

    map<UINT32, RtnStats>::iterator it = rtnstats_map.find(rid);
    if(it!= rtnstats_map.end()){
      //check if there is recursive call
      if( (it->second).entered){
	//PANIC("recursively call %s",(*rtn).c_str());
	ALERT("recursively call %s",(*rtn).c_str());
      }else{

	(it->second).entered = true;
	(it->second).visit  ++;
	(it->second).memops_snapshot = (dynamic_read_ins+dynamic_write_ins);
	(it->second).ins_snapshot = rtn_ins_cnt;
      }
    }else{

      RtnStats r;
      r.id = rid;
      r.name = *rtn;
      r.memops = 0;
      r.ins = 0;
      r.entered = true;
      r.visit = 1;
      r.memops_snapshot = (dynamic_read_ins+dynamic_write_ins);
      r.ins_snapshot = rtn_ins_cnt;
      rtnstats_map.insert(make_pair(rid, r));
    }
}

VOID end_track_rtn(const string* rtn, UINT32 rid) {

#ifdef VERBOSE
  cout << "end tracing rtn "<< *rtn << endl;
#endif

  map<UINT32, RtnStats>::iterator it;
  it = rtnstats_map.find(rid);

  if(it== rtnstats_map.end() || !(it->second).entered ){
    PANIC(" never entered %s",(*rtn).c_str());
  }else{
	(it->second).entered = false;
	//accumulate all memops and arithmetic ins 
	//in all visits to this routine
	(it->second).memops  += ((dynamic_read_ins+dynamic_write_ins) - (it->second).memops_snapshot);
	(it->second).ins     += (rtn_ins_cnt - (it->second).ins_snapshot);
  }

  //need to check all track func is off
  is_record_rtn = false;    
  for(it=rtnstats_map.begin();it!=rtnstats_map.end();it++){
    if( (it->second).entered ){
      is_record_rtn = true;
      break;
    }
  }
}

VOID track_callpath(VOID *rtnAddr, FuncEvent eventType, 
		    ADDRINT returnIp,const string* rtn_name,THREADID tid)
{

  if(!is_record_img || !is_track_call) return;
  
  ThreadDataType* t_data = get_tls(tls_key, tid);
  MetaObj&  curr_memobj  = t_data->curr_allocobj;

  //if(curr_memobj.var_name.find_first_not_of(" \t\n\v\f\r")==string::npos) return;
  if(curr_memobj.creator_tid <0) return;//if no object in tracking
  
    string curr_filename;
    INT32  curr_line;
    INT32  curr_column;
    PIN_LockClient();
    PIN_GetSourceLocation(returnIp,&curr_column, &curr_line, &curr_filename);
    PIN_UnlockClient();
    if(curr_filename.find(USR_HOME_DIR)==string::npos) return;
  
  if(eventType == FUNC_OPEN){
#ifdef DEBUG
      cout << "Enter Routine: " << *rtn_name<< ", file: "<<curr_filename<<", line: "<<curr_line<<endl;
#endif

      is_track_call = false;
    
      curr_memobj.type     = HEAP_T;
      curr_memobj.obj_id   = masterlist.size();
      curr_memobj.start_time  = gettime_in_sec();
      curr_memobj.st_ins      = global_ins_cnt;
      curr_memobj.st_memins   = dynamic_read_ins+dynamic_write_ins;

      if(curr_memobj.source_code_info.find_first_not_of(" \t\n\v\f\r") != string::npos){
	//Update Shared Data structures
	PIN_GetLock(&lock, tid+1);
	masterlist.push_back(curr_memobj);

	//add to active list
	MetaObj& a = masterlist.back();
	TraceObjMeta b( a );
	activelist.push_back(b);
	PIN_ReleaseLock(&lock);
#ifdef VERBOSE
	a.print_meta();
#endif
      }

      //reset tracking
      curr_memobj.reset();
      (t_data->open_alloc_list).clear();
  }
    
    if(eventType == FUNC_CLOSE){
#ifdef DEBUG
      cout << "Exit Routine: " << *rtn_name<< ", file: "<<curr_filename<<", line: "<<curr_line<<endl;
#endif

      stringstream ss;
      ss << curr_filename << ":"<< curr_line;
      string key = ss.str();
      string var_str;
      map<string, string>::iterator vit = var_map.find(key);
      if(vit==var_map.end()){
	std::ifstream file(curr_filename.c_str());
	if(file.is_open())
	{
	  file.seekg(ios::beg);
	  string ll;
	  int extend_search_line = 2;
	  int lnum=0;
	  while(lnum ++ < curr_line) getline(file, ll);

	  while( ll.length()==0 && (lnum ++ < (curr_line+extend_search_line))) 
	    getline(file, ll); //allow search one line before and after 

	  lnum=curr_line-extend_search_line;
	  while( ll.length()==0 && (lnum ++ <curr_line)) 
	    getline(file, ll); //allow search one line before and after 
		
	  if(ll.length()==0) ll = "";

	  size_t head = ll.find_first_not_of(" \t\n\v\f\r");
	  size_t tail = ll.find_last_not_of(" \t\n\v\f\r");
	  ll = ll.substr(head, tail-head);
	  var_map.insert(make_pair(key, ll));
	  var_str = ll;
#ifdef DEBUG
	  cout << "Insert to var_map "<< key << ":" << var_str<<endl;
#endif
	}
      }else{
	  var_str = vit->second;
      }

      curr_memobj.var_name = var_str;
      if(curr_memobj.source_code_info.size()>0) curr_memobj.source_code_info.append("\n");
      curr_memobj.source_code_info.append(ss.str());
    }

}

//this is used for global timestamp
VOID countBBLIns(UINT64 c) {
  global_ins_cnt += c;
}

//this is used for memory intensity in routines
VOID countIns(THREADID tid) {
  if(!is_record_img || !is_record_rtn) return;
  rtn_ins_cnt ++;
  
  ThreadDataType* t_data = get_tls(tls_key, tid);
  t_data->ins ++;
}


/* ===================================================================== */
// Instrumentation Functions
/* ===================================================================== */

//instrument allocation functions
//mark the tracked functions
VOID Image(IMG img, VOID *v)
{
    
#ifdef DEBUG
    cout << "Current loaded Image Name: " << IMG_Name(img)<< ", isMainImage=" << IMG_IsMainExecutable(img) <<endl;
#endif
    

    //Only enable the tracing in the main rtn
    if(IMG_IsMainExecutable(img)){
    
        img_addr_low =  IMG_LowAddress(img);
        img_addr_high=  IMG_HighAddress(img);

        RTN mainRtn = RTN_FindByName(img,"main");
	if( RTN_Valid(mainRtn) )
	{
	  RTN_Open(mainRtn);
	  RTN_InsertCall(mainRtn, IPOINT_BEFORE, (AFUNPTR)start_trace_main,
			 IARG_PTR, RTN_Address(mainRtn),
			 IARG_RETURN_IP, IARG_END);
	  RTN_InsertCall(mainRtn, IPOINT_AFTER,  (AFUNPTR)end_trace_main,
			 IARG_PTR, RTN_Address(mainRtn),
			 IARG_RETURN_IP, IARG_END);
	  RTN_Close(mainRtn);    
	}
    }


    //Check HOOK; HOOK has higher priority than tracked routines
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
      {
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn))
	  {
            RTN_Open( rtn );
            
            for( INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins) )
	      {
		if (INS_IsXchg(ins) && INS_OperandReg(ins, 0) == REG_RCX && INS_OperandReg(ins, 1) == REG_RCX) {
		  INS_InsertCall(ins,IPOINT_BEFORE,(AFUNPTR) HookFunc, IARG_THREAD_ID, IARG_REG_VALUE,REG_ECX, IARG_END);
		  is_record_rtn = false;//when hooks exist, only trace inside hook
		}
	      }

            RTN_Close( rtn );
	  }
      }

    if( list_track_func.size()>0){
      
      //Iterate all tracking routines of interest
      vector<string>::iterator it;
      for(it=list_track_func.begin();it!=list_track_func.end();++it){
	
	  RTN track_rtn = RTN_FindByName(img, it->c_str());
        
	  if(!RTN_Valid(track_rtn)){
	    //Option B: iterate all symbols undecorated name 
	    //Walk through the symbols in the symbol table.
	    //compare with the routines of interest
	    for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
	      {
		string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
		if(undFuncName.find("<")!=string::npos)
		  undFuncName = undFuncName.substr(0,undFuncName.find("<"));
		
		if(undFuncName.find(*it) == 0){
		  track_rtn  = RTN_FindByName(img,SYM_Name(sym).c_str());
		  break;
		}
	      }
	  }

	  if(RTN_Valid(track_rtn)){
#ifdef VERBOSE
            cout << "Located tracked routine: " << *it <<endl;
#endif            
	    RTN_Open(track_rtn);
	    RTN_InsertCall(track_rtn, IPOINT_BEFORE, (AFUNPTR)start_track_rtn,
			   IARG_PTR, new string(*it), 
			   IARG_UINT32, RTN_Id(track_rtn), IARG_END);
	    RTN_InsertCall(track_rtn, IPOINT_AFTER,  (AFUNPTR)end_track_rtn,
			   IARG_PTR, new string(*it),  
			   IARG_UINT32, RTN_Id(track_rtn), IARG_BOOL,false,IARG_END);
	    RTN_Close(track_rtn);
      	  }
      }
    }

    //Iterate all allocation routines
    if(list_alloc_func.size()>0){

#ifdef false
      vector<AllocFunc>::iterator it0;
      for(it0=list_alloc_func.begin(); it0<list_alloc_func.end(); it0++)
	printf("Alloc function: %s %s\n", (it0->name).c_str(),
	       (it0->isInternal ?"Internal" : "External"));
#endif
     
	UINT32 fid=0;
	vector<AllocFunc>::iterator it;
	for(it=list_alloc_func.begin();it!=list_alloc_func.end();++it)
        {	  
	    AllocFunc& func = *it;
	    //Option A: iterate all allocation routines of interest
            RTN allocRtn = RTN_FindByName(img, it->name.c_str());

	    if(!RTN_Valid(allocRtn))
	      {
		//Option B: iterate all symbols undecorated name 
		//Walk through the symbols in the symbol table.
		//compare with the routines of interest
		for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
		  {
		    string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
		    if(undFuncName.find("<")!=string::npos)
		      undFuncName = undFuncName.substr(0,undFuncName.find("<"));

		    if(undFuncName.compare( it->name )==0){

		      //RTN allocRtn1 = RTN_FindByAddress(IMG_LowAddress(img) + SYM_Value(sym));
		      allocRtn  = RTN_FindByName(img,SYM_Name(sym).c_str());
		      break;
		    }
		  }
	      }

	    if( RTN_Valid(allocRtn) )
	      {
#ifdef DEBUG
	      cout << "Located allocation routine: " << it->name
		   <<"(" <<func.argid_itemsize
		   << ","<<func.argid_itemnum
		   << ","<<func.argid_ptridx<< ")" <<endl;
#endif
	      
                RTN_Open(allocRtn);

                //check the signature of the allocation function
                if(func.argid_itemsize < 0)
                    RTN_InsertCall(allocRtn, IPOINT_BEFORE, (AFUNPTR)AllocBefore,
                                   IARG_ADDRINT, 1,
                                   IARG_FUNCARG_ENTRYPOINT_VALUE, func.argid_itemnum,
                                   IARG_RETURN_IP, IARG_THREAD_ID, 
				   IARG_UINT32, fid, IARG_END);
                else
                    RTN_InsertCall(allocRtn, IPOINT_BEFORE, (AFUNPTR)AllocBefore,
                                   IARG_FUNCARG_ENTRYPOINT_VALUE, func.argid_itemsize,
                                   IARG_FUNCARG_ENTRYPOINT_VALUE, func.argid_itemnum,
                                   IARG_RETURN_IP, IARG_THREAD_ID, 
				   IARG_UINT32, fid, IARG_END);
                
                if(func.argid_ptridx < 0)
                    RTN_InsertCall(allocRtn, IPOINT_AFTER, (AFUNPTR)AllocAfter,
                                   IARG_FUNCRET_EXITPOINT_VALUE,
                                   IARG_RETURN_IP, IARG_THREAD_ID,
                                   IARG_UINT32, fid, IARG_END);
                    
                else
                    RTN_InsertCall(allocRtn, IPOINT_AFTER, (AFUNPTR)AllocAfter,
                                   IARG_FUNCARG_ENTRYPOINT_VALUE, func.argid_ptridx,
                                   IARG_RETURN_IP, IARG_THREAD_ID,
                                   IARG_UINT32, fid, IARG_END);
                
                RTN_Close(allocRtn);
            }
	    fid ++;	    
        }
    }//End of all allocation routines

    
    //Iterate all free routines
    if(list_free_func.size()>0){

#ifdef DEBUG
      vector<AllocFunc>::iterator it0;
      for(it0=list_free_func.begin(); it0<list_free_func.end(); it0++)
	printf("Free function: %s %s\n", (it0->name).c_str(),
	       (it0->isInternal ?"Internal" : "External"));
#endif


      if(KnobFreeFuncFile.Value().length()==0){
        
        vector<AllocFunc>::iterator it;
	for(it=list_free_func.begin();it!=list_free_func.end();++it){        

	  RTN freeRtn = RTN_FindByName(img,  it->name.c_str());
	  if(RTN_Valid(freeRtn)){
#ifdef DEBUG
          cout << "Located free routine: "<<  it->name <<endl;
#endif      
            RTN_Open(freeRtn);
            RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)FreeBefore,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_RETURN_IP,IARG_THREAD_ID, IARG_END);
            RTN_Close(freeRtn);
	  }
	}
      }else{
	
	for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
	  {
	    string undFuncName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
	    if(undFuncName.find("<")!=string::npos)
	      undFuncName = undFuncName.substr(0,undFuncName.find("<"));

	    vector<AllocFunc>::iterator it;
	    for(it=list_free_func.begin();it!=list_free_func.end();++it)
	      {
		if(undFuncName.find( it->name) == 0){

		  RTN freeRtn  = RTN_FindByName(img,SYM_Name(sym).c_str());

		  if(RTN_Valid(freeRtn) )
		    {
#ifdef DEBUG
		      cout << "Located free routine: "<<  it->name<< 
			", undFuncName:" <<undFuncName<< ", SYM_Name: "<<SYM_Name(sym) <<endl;
#endif		
		      RTN_Open(freeRtn);
		      RTN_InsertCall(freeRtn, IPOINT_BEFORE, (AFUNPTR)FreeBefore,
				     IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
				     IARG_RETURN_IP,IARG_THREAD_ID, IARG_END);
		      RTN_Close(freeRtn);
		    }
		}
	      }
	  }
      }
    }
    
    /*else if(list_hooks.size()>0){

      if(list_hooks.size()!=2){
	PANIC("Only a pair of hooks can be setup, received %lu hooks", list_hooks.size());
      }
      RTN hookopen_rtn = RTN_FindByName(img, list_hooks[0].c_str());
      RTN hookclose_rtn= RTN_FindByName(img, list_hooks[1].c_str());
     
	if(RTN_Valid(hookopen_rtn)){
#ifdef VERBOSE
	cout << "Located hook: " << list_hooks[0]<<endl;
#endif

	RTN_Open(hookopen_rtn);
	RTN_InsertCall(hookopen_rtn, IPOINT_BEFORE, (AFUNPTR)start_track_rtn,
		       IARG_PTR, new string( list_hooks[0]),
		       IARG_UINT32, 0, IARG_END);
	RTN_Close(hookopen_rtn);
	}
	if(RTN_Valid(hookclose_rtn)){
#ifdef VERBOSE
	cout << "Located hook: "<< list_hooks[1]<<endl;
#endif
	RTN_Open(hookclose_rtn);
	RTN_InsertCall(hookclose_rtn, IPOINT_BEFORE, (AFUNPTR)end_track_rtn,
		       IARG_PTR, new string( list_hooks[1]),
		       IARG_UINT32, 1,IARG_BOOL,true,IARG_END);
	RTN_Close(hookclose_rtn);
	}
    }else{
        is_record_rtn = true;
    }*/

}


//instrument only if the user requires
//tracking allocation-centric callpath
VOID Routine(RTN rtn, VOID * v){

  if(!RTN_Valid(rtn))return;  
  string funcName = RTN_Name(rtn);
  if(funcName.compare(0,5,".text")==0) return;

  string undFuncName = PIN_UndecorateSymbolName(funcName,UNDECORATION_NAME_ONLY);

  RTN_Open(rtn);
  RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)track_callpath,
		 IARG_PTR, RTN_Address(rtn), 
		 IARG_UINT32, FUNC_OPEN,
		 IARG_RETURN_IP,IARG_PTR,new string(undFuncName),
		 IARG_THREAD_ID, IARG_END);
  RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)track_callpath,
		 IARG_PTR, RTN_Address(rtn),
		 IARG_UINT32, FUNC_CLOSE, 
		 IARG_RETURN_IP,IARG_PTR,new string(undFuncName),
		 IARG_THREAD_ID, IARG_END);
  RTN_Close(rtn);
}


//instrument all read and write accesses
//accesses to the stack are excluded
//prefetch accesses are included
VOID Trace(TRACE trace, VOID * val)
{
  RTN rtn = TRACE_Rtn(trace);
  if(!RTN_Valid(rtn)) return;

  IMG img = SEC_Img(RTN_Sec(rtn));
  if(!IMG_Valid(img)) return;//to avoid image stale error


  for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
  {
    //coaser granulairy than instrumenting instructions
    BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)countBBLIns, IARG_UINT64, BBL_NumIns(bbl), IARG_END);

    for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
    {
      //Instrument memops
      if(!INS_IsStackRead(ins) && !INS_IsStackWrite(ins) &&
	 (INS_IsStandardMemop(ins) || INS_HasMemoryVector(ins)))
      {
          
          if(INS_IsMemoryRead(ins)){
              static_read_ins ++;
              INS_InsertPredicatedCall(ins, IPOINT_BEFORE,(AFUNPTR)TraceMem,
                                       IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
                                       IARG_BOOL, true, IARG_THREAD_ID, IARG_END);
          }
          
          if(INS_HasMemoryRead2(ins)){
              static_read_ins ++;
              INS_InsertPredicatedCall(ins,IPOINT_BEFORE, (AFUNPTR)TraceMem,
                                       IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE,
                                       IARG_BOOL, true, IARG_THREAD_ID, IARG_END);
          }
          
          if(INS_IsMemoryWrite(ins)){
              static_write_ins ++;
              INS_InsertPredicatedCall(ins,IPOINT_BEFORE, (AFUNPTR)TraceMem,
                                       IARG_MEMORYWRITE_EA,IARG_MEMORYWRITE_SIZE,
                                       IARG_BOOL, false, IARG_THREAD_ID, IARG_END);
          }
      }

	/*Instrument specific arithmetic ins
	const OPCODE op = INS_Opcode(ins);
	for(auto o : op_list)
	if(op==o){
	  //cout << "Ins" << INS_Mnemonic(ins) << " Op_code " << op<< endl;
	  INS_InsertPredicatedCall(ins,IPOINT_BEFORE,(AFUNPTR)countIns,  IARG_THREAD_ID, IARG_END);
	  break;
	}*/

	//INS_InsertPredicatedCall(ins,IPOINT_BEFORE,(AFUNPTR)countIns, IARG_THREAD_ID, IARG_END); 
    }
  }
}


//init a thread-private data structure when a thread starts
VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
  if(PIN_GetThreadData(tls_key,tid)==NULL){
  
    ThreadDataType* t_data = new ThreadDataType();

    if(KnobTracer || KnobOutputMemTrace){

      stringstream ss;
      ss<<memtrace_file_name<<".t"<<tid<<".tr";
      FILE *file = fopen(ss.str().c_str(), "ab");
      //if(file) PANIC("File %s already exists",ss.str().c_str());

      t_data->ins = 0;
      t_data->tfile = file;
      t_data->buf   = new ACCESS_T[BUF_SIZE];
      t_data->buf_pos= 0;
      t_data->total = 0;
    }

    PIN_SetThreadData(tls_key, t_data, tid);
  }
  
  thread_map[tid] = 1;

#ifdef VERBOSE
    cout << "Thread " << tid << " [" << syscall(SYS_gettid) << "] is starting\n";
#endif
    
}


//push back a thread's accesses to the master list when it ends
VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    //mark it as inactive
    if(thread_map.find(tid)==thread_map.end())
      PANIC("Tid %i is never activated.",tid);
    thread_map[tid]=0;
    
    double t = gettime_in_sec();

#ifdef VERBOSE
    cout << "ThreadFini(tid="<< tid << ") " << endl << flush;
#endif

    ThreadDataType* t_data = get_tls(tls_key, tid);

    if(KnobTracer || KnobOutputMemTrace){ 
      cout << "Closing trace t"<<tid<<", total "<<(t_data->total+t_data->buf_pos)<<endl;
      fwrite(t_data->buf, sizeof(ACCESS_T),t_data->buf_pos,t_data->tfile);
      fclose(t_data->tfile);
      delete[] t_data->buf;
    }

    map<int, TraceObjThreadAccess>::iterator it;
  
    PIN_GetLock(&lock, tid+1);
    for(it=(t_data->accessed_objects).begin(); it!=(t_data->accessed_objects).end(); ++it){

#ifdef DEBUG
      cout << "Threadid "<< tid << " has accessed Obj Id " << it->first << endl << flush;
#endif
        
      masterlist[it->first].add_thread_accesses(tid, it->second, t);
        
    }
    PIN_ReleaseLock(&lock);

    //delete thread data
    PIN_SetThreadData(tls_key, 0, tid);
}

//flush all the metrics to the analyzer when program ends
VOID Fini(INT32 code, VOID *v)
{
  double time_end= gettime_in_sec();

  //for early detach
  if(hasCalledFini) return; 
  hasCalledFini = true;  

  cout<< "Finalizing..."<<endl;
  cout<< "Profiling time: "<< (time_end-time_start) <<" seconds. "<<rtn_ins_cnt<<endl;
 

  //finalize active threads
  map<THREADID, int>::iterator it0;
  for(it0=thread_map.begin();it0!=thread_map.end();++it0)
    if(it0->second==1) 
      ThreadFini(it0->first,NULL,0, NULL);
  //thread_map.clear();

  vector<RtnStats> rtn_list;
  map<UINT32, RtnStats>::iterator it3;
  for(it3=rtnstats_map.begin();it3!=rtnstats_map.end();it3++){
      rtn_list.push_back(it3->second);
  }
  sort(rtn_list.begin(), rtn_list.end(), compare_by_memintensity);
  for(auto ii : rtn_list){
    cout << "Visited " << ii.name<< " " << ii.visit 
	 << " times, memops "<< ii.memops << ", ins " << ii.ins 
	 << ", memops/ops "<<((double)ii.memops)/(ii.ins) <<endl;
  }
    

  if( !KnobTracer ){

  //By right, there should be no objects in the active list
  //if there are any objects, this causes memory leakage
  vector<TraceObjMeta>::iterator it;
  UINT64 ins     = global_ins_cnt;
  UINT64 mem_ins = (dynamic_read_ins+dynamic_write_ins);
  for(it=activelist.begin();it!=activelist.end(); ++it){
      
#ifdef DEBUG
      if(masterlist[it->obj_id].source_code_info.compare("Main Image")!=0)
        cout << "Obj Id "<< it->obj_id << " created at "
             << masterlist[it->obj_id].source_code_info<< ", not freed!\n";
#endif

      masterlist[it->obj_id].release(ins, mem_ins, time_end);
  }

    
  //flush Open Alloc Obj
  if( open_allocobj_list.size()>0 ){

      vector<OpenAllocObj>::iterator it;
      for(it=open_allocobj_list.begin();it!=open_allocobj_list.end();++it){
          //skip level 0 as it has been captured
          for(int level=1;level<it->rtn_level;level++){
	    //cout << "level "<< level<<": " << it->rtn_stack[level]  <<endl;
	    masterlist.at(it->obj_id).source_code_info.append(it->rtn_stack[level]);
          }
      }
  }
    
  //Perform Analysis or save the log file
  GlobalMetrics globalmetrics(static_read_ins,  static_write_ins,
			      dynamic_read_ins, dynamic_write_ins,
			      ins, list_alloc_func);
    
  Analyzer a(globalmetrics, memory_list,masterlist);
  a.analyzeAll();
  a.print_analysis();
  //a.visualize();
  }

  if(KnobCache){
    cout<< l3->StatsLong();
    delete(l3);
  }

  /*   
    Allocator b = new Allocator(globalmetrics, memory_list);
    b.allocate(masterlist);
    b.output(output_file);
    output_file.close();
  */ 

  cout << "Profling is completed !\n" << endl;
}

void handler(int sig) {
  PIN_LockClient();

  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);
  strings = backtrace_symbols (array, size);

  printf ("Obtained %zd stack frames.\n", size);
  for (i = 0; i < size; i++)
    printf ("%s\n", strings[i]);

  PIN_UnlockClient();
}


int main(int argc, char *argv[]){

    PIN_InitSymbols();
    if (PIN_Init(argc, argv)){
        cerr << "Failed to Init Pin.\n";
        return Usage();
    }

    //Open the file for output
    if(KnobTracer || KnobOutputMemTrace){
      stringstream ss;
      if(KnobOutputFile.Value().length()>0) 
	ss <<  KnobOutputFile.Value() << "." << PIN_GetPid();
      else
	ss << "out" << "." << PIN_GetPid();
      memtrace_file_name = ss.str();
    }

    //Init global variables
    dynamic_read_ins=0;
    dynamic_write_ins=0;
    static_read_ins=0;
    static_write_ins=0;
    is_record_img=false;
    is_record_rtn=false;
    is_track_call=false;
    hasCalledFini=false;
    global_ins_cnt=0;
    rtn_ins_cnt=0;
    if(KnobCache){
      l3 = new L3::CACHE("L3 Cache");
      isL3WriteThrough  = l3->get_isWriteThrough();
      isL3WriteAllocate = l3->get_isWriteAllocate();
    }else{
      isL3WriteThrough = true;
      isL3WriteAllocate= false;
    }
    //Read in the user selected functions for tracking
    string trackfilename(KnobTrackFuncFile.Value());
    setup_trackfunclist(list_track_func,trackfilename);
    //when no rtn is selected, track all
    if(list_track_func.size()==0)
      is_record_rtn=true;

    /*string hooksfilename(KnobHook.Value());
    hook_num = setup_hooks(list_hooks, hooksfilename);

    if(list_track_func.size()>0 && list_hooks.size()>0){
      PANIC("Have %lu track_func and %lu hooks. Only one can be enabled.",
	    list_track_func.size(),  list_hooks.size());
    }*/
    hook_num = KnobHook;
#ifdef VERBOSE
    cout << "Track Hooks "<< KnobHook  <<" times"<<endl;
#endif



    //Read in the user defined allocation routines
    string allocfilename(KnobAllocFuncFile.Value());
    setup_allocfunclist(list_alloc_func, allocfilename);
    if(list_alloc_func.size()==0)
      setup_allocfunclist(list_alloc_func);
    //else setup_allocfunclist_internal(list_alloc_func);
    

    //Read in the user defined free routines
    string freefilename(KnobFreeFuncFile.Value());
    setup_freefunclist(list_free_func,freefilename);
    
    //Read in the configuration of main memory
    string memory_config(KnobMemConfigFile.Value());
    setup_memorylist(memory_list,memory_config);
    
    //Find static allocations
    get_static_allocation(masterlist, activelist);
    //upd_static_allocation(masterlist);
    
    string sse("");
    setup_arithmeticinstruction(op_list, sse);

    //Get stack range
    //get_stack_range(&stack_bound_low, &stack_bound_high);
    //printf("Process Stack [%#lx  %#lx] \n",stack_bound_low,stack_bound_high);
    
    //Different granularity here from coarse to refined
    IMG_AddInstrumentFunction(Image, 0);
    if(KnobTrackCallPath)
      RTN_AddInstrumentFunction (Routine,0);
    TRACE_AddInstrumentFunction(Trace, 0);
    

    //Use thread local storage for thead-level memory accesses
    PIN_InitLock(&lock);
    // Obtain  a key for TLS storage.
    tls_key = PIN_CreateThreadDataKey(0);
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);


    PIN_AddFiniFunction(Fini, 0);
    // Callback functions to invoke before                                                                               
    // Pin releases control of the application                                                                           
    PIN_AddDetachFunction(DetachFunc, 0);
   
    time_start = gettime_in_sec();

    signal(SIGSEGV, handler); 
  
    // Never returns
    PIN_StartProgram();
    return 0;
    
}



/* ===================================================================== */
/* Auxiliary Fuctions */
/* ===================================================================== */


bool compare_by_memintensity(const RtnStats& lhs, const RtnStats& rhs){
  return ((lhs.ins/lhs.memops) > (rhs.ins/rhs.memops));
}


void print_metaobj(vector<MetaObj>& masterlist, ostream& output){
    
    //Header
    output <<HLINE<<endl;
    
    vector<MetaObj>::iterator it;
    for(it=masterlist.begin();it!=masterlist.end(); ++it){
        it->print_meta(output);
    }
    output <<HLINE<<endl;

    output << left
    << setw(8) <<"ObjId"
    <<setw(15)<<"No.Threads"
    <<setw(15)<<"St_ins(Mil)"
    <<setw(15)<<"End_ins(Mil)"
    <<setw(10)<<"Size(MB)"
    <<setw(12)<<"MemRef(Mil)"
    <<setw(10)<<"MemRef(%)"
    <<setw(12)<<"Ins/MemRef"
    <<setw(15)<<"Read(Mil)"
    <<setw(15)<<"Write(Mil)"
    <<setw(10)<<"RW(%)"
    <<setw(15)<<"ReadinCache(%)"
    <<setw(12)<< "Strided(%)"
    <<setw(12)<< "Random(%)"
    <<setw(15)<< "PntChasing(%)"
    <<setw(8)<< "HBM"
    <<setw(8)<< "Cache"
    <<setw(8)<< "DRAM"<<"\n";
    
    for(it=masterlist.begin();it!=masterlist.end(); ++it){
        it->print_stats(output);
    }
}



string FormatAddress(ADDRINT address, RTN rtn)
{
    string s = StringFromAddrint(address);
    
    if (RTN_Valid(rtn))
    {
        IMG img = SEC_Img(RTN_Sec(rtn));
        s+= " ";
        if (IMG_Valid(img))
        {
            s += IMG_Name(img) + ":";
        }
        
        s += RTN_Name(rtn);
        
        ADDRINT delta = address - RTN_Address(rtn);
        if (delta != 0)
        {
            s += "+" + hexstr(delta, 4);
        }
    }
    
    INT32 line;
    string file;
    
    PIN_GetSourceLocation(address, NULL, &line, &file);
    
    if (file != "")
    {
        s += " (" + file + ":" + decstr(line) + ")";
    }
    
    cout<<s<<endl;
    return s;
}

void upd_static_allocation(vector<MetaObj>& masterlist){

  vector<MetaObj>::iterator it;
  for(it=masterlist.begin(); it<masterlist.end();++it){
    if( it->type == STATIC_T ){
      string undFuncName = PIN_UndecorateSymbolName(it->var_name, UNDECORATION_NAME_ONLY);
      //cout << it->var_name<<  " change to " << undFuncName <<endl;
      (it->var_name).clear();
      (it->var_name).append(undFuncName);
    }
  }

}
