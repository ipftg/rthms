#include <algorithm>
#include <sstream>
#include <iomanip>
#include "rthms.h"
#include "rthms_analyzer.h"

#define HLINE "======================================================================"
#define MAX2(x,y)  ((x<y) ?y : x)
#define MAX3(x,y,z) ((x<y) ?((y<z) ?z : y ) : ((x<z) ?z : x))

using namespace std;


//sort by id in ascending order
bool compare_by_id_asc(const Memory& lhs, const Memory& rhs) {
    return lhs.memory_id <= rhs.memory_id;
}

//sort by cost in ascending order
bool compare_by_cost_asc(const Memory& lhs, const Memory& rhs) {
    return lhs.cost <= rhs.cost;
}

//sort by the read latency in ascending order
bool compare_by_readlat_asc(const Memory& lhs, const Memory& rhs) {
    return lhs.latency_read_ns <= rhs.latency_read_ns;
}

//sort by the read bw in descending order
bool compare_by_readbw_desc(const Memory& lhs, const Memory& rhs) {
    return lhs.bandwidth_read_gbs >= rhs.bandwidth_read_gbs;
}

//sort by the write bw in descending order
bool compare_by_writebw_desc(const Memory& lhs, const Memory& rhs) {
    return lhs.bandwidth_write_gbs >= rhs.bandwidth_write_gbs;
}


//sort by capacity in ascending order
bool compare_by_capacity_asc(const Memory& lhs, const Memory& rhs) {
    return lhs.capacity_in_mb <= rhs.capacity_in_mb;
}


void Analyzer::aggregate_thread_access(vector<MetaObj>::iterator it)
{
    it->num_threads = 0;
    vector<ThreadMemAccess>::iterator it1;
    if((it->access_list).size()>0)
    for(it1=(it->access_list).begin(); it1<(it->access_list).end();++it1){

#ifdef DEBUG
      cout << "Obj " << it->obj_id <<" accessed by tid " << it1->thread_id <<endl;
#endif

        it->num_threads  ++;
        it->dynamic_read  += it1->dynamic_read;
        it->dynamic_write += it1->dynamic_write;
        it->read_in_cache += it1->read_in_cache;
        it->strided_read  += it1->strided_read;
        it->pointerchasing_read += it1->pointerchasing_read;
        it->random_read   += it1->random_read;
    }
    
    it->mem_ref            = it->dynamic_read+it->dynamic_write;
    it->readwrite_ratio    = it->dynamic_read*100.0/it->mem_ref;
    it->readincache_ratio  = it->read_in_cache*100.0/it->dynamic_read;
    it->read_not_in_cache  = it->dynamic_read - it->read_in_cache;
    it->strided_read_ratio = it->strided_read*100.0/it->read_not_in_cache;
    it->random_read_ratio  = it->random_read *100.0/it->read_not_in_cache;
    it->pointchasing_read_ratio = it->pointerchasing_read*100.0/it->read_not_in_cache;

    totalmetrics.dynamic_read_ins  += it->dynamic_read ;
    totalmetrics.dynamic_write_ins += it->dynamic_write;
}

void Analyzer::set_preference(vector<MetaObj>::iterator it){
    
    //it is possible that multiple objects are created at the same source line
    //consolidate the objects by their source code
    map<string, vector<int> >::iterator find = src_obj_map.find(it->source_code_info);
    
    //obj id instead of pointer to a vector element
    if(find == src_obj_map.end()){
        vector<int> objid_list;
        objid_list.push_back( it->obj_id );
        src_obj_map.insert(make_pair<string, vector<int> >(it->source_code_info,objid_list));

#ifdef DEBUG
	map<string, vector<int>>::const_iterator newit = src_obj_map.find(it->source_code_info);
	cout << it->source_code_info << ", oid " <<  it->obj_id << " is inserted to " << (newit->second).size()<<endl;
#endif
    
    }else{
       //src_obj_map[it->source_code_info].push_back(it->obj_id);
       (find->second).push_back(it->obj_id);

#ifdef DEBUG
	cout << it->source_code_info << ", oid " <<  it->obj_id << " is found to " << (find->second).size()<<endl;
#endif
    }
    
    it->preference_order  = new double[num_mem];
    
    double memory_ins     = (double) it->mem_ref;//(double)(it->end_memins - it->st_memins);
    double non_memory_ins = (double)(it->end_ins - it->st_ins) - memory_ins;
    double non_memory_ins_per_ref = non_memory_ins/memory_ins;
    
    if( it->num_threads==1 || it->mem_ref==0 ||
        non_memory_ins_per_ref>Threshold_CPUIntensity ||
        it->size_in_mb<Threshold_LLCMBSize ){
        
        if(it->num_threads==1){
            (it->preference_reason).append("single-threaded, using the most cost-effective memory");
        }
        else if(it->mem_ref==0){
            (it->preference_reason).append("not accessed, using the most cost-effective memory");
        }
        else if(non_memory_ins_per_ref>Threshold_CPUIntensity){
	    stringstream ss;
	    ss << "compute intensive["<<setw(8)<< non_memory_ins_per_ref << "], using the most cost-effective memory";
            (it->preference_reason).append(ss.str());
        }else{
	    stringstream ss;
	    ss << setw(4) << it->size_in_mb << " MB fits in LLC, using the most cost-effective memory";
            (it->preference_reason).append(ss.str());
        }
        
        //set the cheapest as preferred, others as neutral
        sort(memory_list.begin(), memory_list.end(), compare_by_cost_asc);
        
        int cnt=0;
        vector<Memory>::const_iterator it2;
        for(it2=memory_list.begin(); it2<memory_list.end();++it2){
            
            if(it->size_in_mb > it2->capacity_in_mb) 
	      it->preference_order[it2->memory_id] = UNFIT;
            else{
                if(cnt==0)
		  it->preference_order[it2->memory_id] = 1;// * it->mem_ref_percentage;
                else
                    it->preference_order[it2->memory_id] = 0;
            }
            cnt ++;
        }
        
        return;
    }
    

    if(it->readwrite_ratio<=Threshold_WriteIntensive){
        
      stringstream ss ;
      ss<< "write intensive["<< setw(3)<<it->readwrite_ratio << "], prefer high write BW memory";
        (it->preference_reason).append(ss.str());

        //set the cheapest as preferred, others as neutral
        sort(memory_list.begin(), memory_list.end(), compare_by_writebw_desc);
        
        int cnt=0;
        vector<Memory>::const_iterator it2;
        for(it2=memory_list.begin(); it2<memory_list.end();++it2){
            if(it->size_in_mb > it2->capacity_in_mb) 
	      it->preference_order[it2->memory_id] = UNFIT;
            else{
                if(cnt==0)
                    it->preference_order[it2->memory_id] = 1 * it->mem_ref_percentage;
                else
                    it->preference_order[it2->memory_id] = 0;
            }
            cnt ++;
        }
        
        return;
    }
                        
    //find the dominant access pattern is regular
    bool isRegular = (it->strided_read_ratio==MAX3(it->strided_read_ratio,it->pointchasing_read_ratio,it->random_read_ratio));
    if(isRegular){
        double secondlargest = MAX2(it->pointchasing_read_ratio,it->random_read_ratio);
        bool   isSignificant   = ((secondlargest/it->strided_read_ratio)<Threshold_Significance);
        isRegular = isSignificant;
    }
    
    if( !isRegular ){
        if(it->num_threads < Threshold_Threads){
            
	  stringstream ss;
	  ss<< "irregular["<<left<<setw(5)<<setprecision(2)<<fixed
	    <<(it->pointchasing_read_ratio+it->random_read_ratio)
	    <<"], "<< it->num_threads <<" threads, prefer memory with low read latency ";
            (it->preference_reason).append(ss.str());
            
            //set the lowest latency as preferred, others as negative
            
            sort(memory_list.begin(), memory_list.end(), compare_by_readlat_asc);
            
            int cnt=0;
            vector<Memory>::const_iterator it2;
            for(it2=memory_list.begin(); it2<memory_list.end();++it2){
                if(it->size_in_mb > it2->capacity_in_mb) 
		  it->preference_order[it2->memory_id] = UNFIT;
                else{
                    if(cnt==0)
                        it->preference_order[it2->memory_id] = 1 * it->mem_ref_percentage;
                    else
                        it->preference_order[it2->memory_id] = -1 * it->mem_ref_percentage;
                }
                cnt ++;
            }
            
            return;
            
        }else{
	  stringstream ss;
	  ss<< "irregular["<<left<<setw(5)<<fixed<<setprecision(2)<<(it->pointchasing_read_ratio+it->random_read_ratio)
	    << "], "<< it->num_threads <<" threads, prefer memory with high read bw.";
            
	  (it->preference_reason).append(ss.str());

            //set the highest read bw as preferred, others as negative
            
            sort(memory_list.begin(), memory_list.end(), compare_by_readbw_desc);
            
            int cnt=0;
            vector<Memory>::const_iterator it2;
            for(it2=memory_list.begin(); it2<memory_list.end();++it2){
                if(it->size_in_mb > it2->capacity_in_mb) 
		  it->preference_order[it2->memory_id] = UNFIT;
                else{
                    if(cnt==0)
		        it->preference_order[it2->memory_id] = 1 * it->mem_ref_percentage;
                    else
                        it->preference_order[it2->memory_id] = -1 * it->mem_ref_percentage;
                }
                cnt ++;
            }
            
            return;
        }
    }
    else{
        stringstream ss;
        ss<< "regular["<<left<<setw(5)<< fixed<<setprecision(2) <<(it->strided_read_ratio)
                        << "], "<< it->num_threads <<" threads, prefer memory with high read bw.";
        (it->preference_reason).append(ss.str());
        
        //set the highest read bw as preferred in order
        
        sort(memory_list.begin(), memory_list.end(), compare_by_readbw_desc);
        
        int cnt=1;
        vector<Memory>::const_iterator it2;
        for(it2=memory_list.begin(); it2<memory_list.end();++it2){
            if(it->size_in_mb > it2->capacity_in_mb) 
	      it->preference_order[it2->memory_id] = UNFIT;
            else{
                if(it->size_in_mb < (it2->capacity_in_mb * it2->locality_overhead_threshold))
                    it->preference_order[it2->memory_id] = (num_mem - cnt) *  it->mem_ref_percentage;
                else
                    it->preference_order[it2->memory_id] = -1 *  it->mem_ref_percentage;
            }
            cnt ++;
        }
        
        return;
    }
}

void Analyzer::analyze(vector<MetaObj>::iterator it){
    

}

void Analyzer::print_preference(vector<MetaObj>::iterator obj, ostream& output){
            
  output << "Prefernce     :" << obj->preference_reason << " [";
  
  vector<Memory>::const_iterator it;
  for(it=memory_list.begin(); it<memory_list.end();++it){
    output << setw(10)<< fixed<<setprecision(2) << obj->preference_order[it->memory_id] << " ";
  }
  output << "]" << endl;
}


void Analyzer::analyzeAll(){

    totalmetrics.dynamic_read_ins = 0 ;
    totalmetrics.dynamic_write_ins = 0;
#ifdef DEBUG
    cout<<"Before aggregate_thread_access"<<endl;
#endif
    vector<MetaObj>::iterator it;
    for(it=objlist.begin();it!=objlist.end(); ++it){
      aggregate_thread_access(it);
    }
#ifdef DEBUG
    cout<<"after aggregate_thread_access"<<endl;
    cout<< "Global static_read="<<totalmetrics.static_read_ins <<", static_write = "<<totalmetrics.static_read_ins
	<< "dynamic_read="<<totalmetrics.dynamic_read_ins<<", dynamic_write = "<<totalmetrics.dynamic_write_ins<<endl;
#endif

    for(it=objlist.begin();it!=objlist.end(); ++it){
      //set other derived metrics
      it->size_in_mb   = (double)it->size*1.E-6;
      it->mem_ref_percentage = it->mem_ref*100.0/(totalmetrics.dynamic_read_ins+totalmetrics.dynamic_write_ins);
    
#ifdef VERBOSE
      it->print_meta();
      cout << "\n";
      it->print_stats();
#endif

      set_preference(it);
    
#ifdef VERBOSE
      print_preference(it);
      cout <<endl;
#endif
      
    }
}



void Analyzer::print_analysis(ostream& output){
                                                     
    sort(memory_list.begin(), memory_list.end(), compare_by_id_asc);
  
    output <<HLINE<<endl;
    output <<left<<setw(100)<<"Per Object Analysis:"; 
    for (int i=0; i<num_mem; i++)
      output<< left << setw(6)<<memory_list[i].memory_name <<" ";
    output <<endl<<HLINE<<endl;
    
    map<string, vector<int> >::iterator it;
    for(it=src_obj_map.begin();it!=src_obj_map.end(); ++it){
        
        output<< it->first <<endl;
        vector<int>::iterator it2;
        for(it2=(it->second).begin();it2!=(it->second).end(); ++it2){
	  size_t padding=0;
	  stringstream ss;
	  if(objlist[*it2].var_name.length() >0 )
	    ss << "OId " << setw(4)<< *it2 <<": <"<< objlist[*it2].var_name <<">: ";
	  else
	    ss << "OId " << setw(4)<< *it2 <<": ";
	  ss << objlist[*it2].preference_reason << ": ";
	  padding = 100-ss.str().length();
	  ss<<setw(padding)<<" ";
	  for (int i=0; i<num_mem; i++) {
	    ss<<left<< setw(3)<<fixed<<setprecision(1)<< objlist[*it2].preference_order[i] << setw(3)<<" ";
	  }
	  cout << ss.str() << endl;
        }
        
        output <<HLINE<<endl;
    }
}

void Analyzer::visualize(ostream& output){
                                                     
    sort(memory_list.begin(), memory_list.end(), compare_by_id_asc);
    output <<HLINE<<endl;
    output <<"For Vis"<<endl;

    int map_id=0;
    map<string, vector<int> >::iterator it;
    for(it=src_obj_map.begin();it!=src_obj_map.end(); ++it){
      vector<int>::iterator it2;
      for(it2=(it->second).begin();it2!=(it->second).end(); ++it2){
	stringstream ss;
	ss<<map_id<<";"<<it->first<<";"     <<objlist[*it2].obj_id <<";"<<objlist[*it2].st_ins*10e-6  <<";"
	  <<objlist[*it2].end_ins*10e-6<<";"<<objlist[*it2].mem_ref<<";"<<(objlist[*it2].dynamic_read)<<";"
	  <<objlist[*it2].dynamic_write<<";"<<objlist[*it2].readincache_ratio<<endl;
	output << ss.str();
      }
      map_id++;  
    }
}

