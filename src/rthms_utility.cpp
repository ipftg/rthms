#include <sstream>
#include <fstream>
#ifdef DEBUG
#include <algorithm>
#endif

#include "rthms_utility.h"
using namespace std;

/*
    std::string  name;
    int    argid_itemsize;
    int    argid_itemnum;
    int    argid_ptridx;
*/
VOID setup_allocfunclist(vector<AllocFunc> &allocfunclist, string& filename){

  //if the user provides wraper functions
  if( filename.length()>0)
    {  
      //setup the list of selected routines
      std::ifstream file(filename.c_str());
      if ( !file.is_open() ) {
	PANIC("cannot open %s", filename.c_str());
      }

      std::string   line;
      while(getline(file, line)){
	stringstream   linestream(line);
	string fn;
	int id0, id1,id2;
	linestream >> fn  >> id0 >> id1 >> id2;

	if(fn.length()>0){
	  AllocFunc uf(fn,  id0, id1, id2, false);
	  allocfunclist.push_back(uf);
	}
      }
    }
}


VOID setup_allocfunclist(vector<AllocFunc> &allocfunclist){

    //add all default functions
  
    //ptr = malloc(size_t)
    string a("malloc");
    AllocFunc aa(a, -1, 0, -1, false);
    allocfunclist.push_back(aa);

    //ptr = calloc(size_t num, size_t size);
    string b("calloc");
    AllocFunc bb(b, 0, 1 , -1, false);
    allocfunclist.push_back(bb);

    //void* realloc (void* ptr, size_t size);
    string c("realloc");
    AllocFunc cc(c, -1, 1, -1, false);
    allocfunclist.push_back(cc);

    //void *valloc(size_t size);
    string d("valloc"); 
    AllocFunc dd(d, -1, 0, -1, false);
    allocfunclist.push_back(dd);

    //void *pvalloc(size_t size);
    string e("pvalloc");
    AllocFunc ee(e, -1, 0, -1, false);
    allocfunclist.push_back(ee);

    //void *memalign(size_t alignment, size_t size);
    string f("memalign");
    AllocFunc ff(f, -1, 1, -1, false);
    allocfunclist.push_back(ff);

    //int posix_memalign(void **memptr, size_t alignment, size_t size);
    string g("posix_memalign");
    AllocFunc gg(g, -1, 2, 0, false);
    allocfunclist.push_back(gg);
}

VOID setup_allocfunclist_internal(vector<AllocFunc> &allocfunclist){

    //add all default functions
  
    //ptr = malloc(size_t)
    string a("malloc");
    AllocFunc aa(a, -1, 0, -1, true);
    allocfunclist.push_back(aa);

    //ptr = calloc(size_t num, size_t size);
    string b("calloc");
    AllocFunc bb(b, 0, 1 , -1, true);
    allocfunclist.push_back(bb);

    //void* realloc (void* ptr, size_t size);
    string c("realloc");
    AllocFunc cc(c, -1, 1, -1, true);
    allocfunclist.push_back(cc);

    //void *valloc(size_t size);
    string d("valloc"); 
    AllocFunc dd(d, -1, 0, -1, true);
    allocfunclist.push_back(dd);

    //void *pvalloc(size_t size);
    string e("pvalloc");
    AllocFunc ee(e, -1, 0, -1, true);
    allocfunclist.push_back(ee);

    //void *memalign(size_t alignment, size_t size);
    string f("memalign");
    AllocFunc ff(f, -1, 1, -1, true);
    allocfunclist.push_back(ff);

    //int posix_memalign(void **memptr, size_t alignment, size_t size);
    string g("posix_memalign");
    AllocFunc gg(g, -1, 2,  0, true);
    allocfunclist.push_back(gg);

}


/*
    std::string  name;
    int    argid_itemsize;
    int    argid_itemnum;
    int    argid_ptridx;
*/
VOID setup_freefunclist(vector<AllocFunc> &freefunclist, string& filename){

  string a("free");
  AllocFunc aa(a, -1, -1, 0, false);
  freefunclist.push_back(aa);

  if( filename.length()>0)
    {
      
      //setup the list of selected routines
      std::ifstream file(filename.c_str());
      if ( !file.is_open() ) {
	PANIC("cannot open %s", filename.c_str());
      }

      std::string   line;
      while(getline(file, line)){
	stringstream   linestream(line);
	string fn;
	int id0, id1,id2;
	linestream >> fn  >> id0 >> id1 >> id2;
      
	if(fn.length()>0){
	  AllocFunc uf(fn,  id0, id1, id2, false);
	  freefunclist.push_back(uf);
	}
      }
    }
}

void setup_memorylist(vector<Memory> &memory_list, string& filename){
    
    if( filename.length()==0){
        Memory a;
        a.memory_id = 0;
        a.memory_name.append("HBM");
        a.cost = 3;
        a.capacity_in_mb = 1024; //1GB HBM
        a.latency_read_ns= 2;
        a.bandwidth_read_gbs=3;
        a.bandwidth_write_gbs=3;
        a.speedup_by_hwt = 3;
        a.locality_overhead_threshold = 1.0;
        memory_list.push_back(a);
        
        Memory b;
        b.memory_id = 1;
        b.memory_name.append("DRAM");
        b.cost = 1;
        b.capacity_in_mb = 96;
        b.latency_read_ns= 1;
        b.bandwidth_read_gbs=1;
        b.bandwidth_write_gbs=1;
        b.speedup_by_hwt = 1;
        b.locality_overhead_threshold = 1.0;
        memory_list.push_back(b);
        
        Memory c;
        c.memory_id = 2;
        c.memory_name.append("Cache");
        c.cost = a.cost;
        c.capacity_in_mb = a.capacity_in_mb + b.capacity_in_mb;
        c.latency_read_ns= 3;
        c.bandwidth_read_gbs=2;
        c.bandwidth_write_gbs=2;
        c.speedup_by_hwt = 2;
        c.locality_overhead_threshold = 0.5;
        memory_list.push_back(c);
        
    }else{
        //read in the list from user input
    }
    
    /*#ifdef DEBUG
    vector<Memory>::iterator it;
    sort(memory_list.begin(), memory_list.end(), compare_by_cost_asc);
    cout << "Sort by cost\n";
    for(it=memory_list.begin(); it<memory_list.end();++it)
        cout << it->memory_name << ", cost " << it->cost << "\n";
    
    sort(memory_list.begin(), memory_list.end(), compare_by_readlat_asc);
    cout << "Sort by read latency in ascending order\n";
    for(it=memory_list.begin(); it<memory_list.end();++it)
        cout << it->memory_name << ", readlat " << it->latency_read_ns << "\n";
    
    sort(memory_list.begin(), memory_list.end(), compare_by_readbw_desc);
    cout << "Sort by read bandwidth in descending order\n";
    for(it=memory_list.begin(); it<memory_list.end();++it)
        cout << it->memory_name << ", readbw " << it->bandwidth_read_gbs << "\n";
    
    sort(memory_list.begin(), memory_list.end(), compare_by_writebw_desc);
    cout << "Sort by write bandwidth in descending order\n";
    for(it=memory_list.begin(); it<memory_list.end();++it)
        cout << it->memory_name << ", writebw " << it->bandwidth_write_gbs << "\n";
#endif
    */
    
    
}

void setup_trackfunclist(vector<string> &trackfunclist, string& filename){
    
  if( filename.length()>0){
  
    //setup the list of selected routines
    std::ifstream file(filename.c_str());
      if ( !file.is_open() ) {
	PANIC("cannot open %s", filename.c_str());
      }

      std::string   line;
      while(getline(file, line)){
	stringstream   linestream(line);
	string rtn;
	linestream >> rtn;
	if(rtn.length()>0){
	  trackfunclist.push_back(rtn);
#ifdef DEBUG
	  cout << "Track Rtn: " << trackfunclist.back()<<endl;
#endif    
	}
      }
  }
}

size_t setup_hooks(vector<string> &hooklist, string& filename){
    
  size_t hook_num = 0;//trace all visits
  if( filename.length()>0){  
    std::ifstream file(filename.c_str());
      if ( !file.is_open() ) {
	PANIC("cannot open %s", filename.c_str());
      }

      int cnt=0;
      std::string   line;
      while(getline(file, line) && cnt<3){
	size_t h = line.find_first_not_of(" \n\t");
	size_t t = line.find_last_not_of(" \n\t");
	if(t<h) break;
	line = line.substr(h,t-h+1);
	stringstream   linestream(line);
	if(cnt<2){
	  string rtn;
	  linestream >> rtn;
	  if(rtn.length()>0){
	    hooklist.push_back(rtn);
	  }
	}else if(cnt==2){
	  int i;
	  linestream >> i;
	  if(i>0) hook_num = i;
	}
#ifdef DEBUG
	  if(cnt==0)
	    cout << "Hook Open: " << hooklist.back()<<endl;
	  else if(cnt==1)
	    cout << "Hook Close: " << hooklist.back()<<endl;
	  else if(cnt ==2)
	    cout << "Visit " <<std::dec<< hook_num << " time "<<endl;
#endif  
	    cnt ++;
      }
  
      if(hooklist.size()!=2){
	PANIC("Only a pair of hooks can be setup, received %lu hooks", hooklist.size());
      }
  }
  return hook_num;
}

void setup_arithmeticinstruction(vector<OPCODE> &a_list, string& filename){
    
    if( filename.length()>0){
    
    
    }else{
      
      a_list.push_back(XED_ICLASS_ADD);
      a_list.push_back(XED_ICLASS_SUB);
      a_list.push_back(XED_ICLASS_MUL);
      a_list.push_back(XED_ICLASS_DIV);


      //Floating-point instructions
      a_list.push_back(XED_ICLASS_FDIV);
      a_list.push_back(XED_ICLASS_FADD);
      a_list.push_back(XED_ICLASS_FMUL);
      a_list.push_back(XED_ICLASS_FSUB);

      //SSE Instructions
      a_list.push_back(XED_ICLASS_ADDSS);
      a_list.push_back(XED_ICLASS_SUBSS);
      a_list.push_back(XED_ICLASS_MULSS);
      a_list.push_back(XED_ICLASS_DIVSS);


    }
}
