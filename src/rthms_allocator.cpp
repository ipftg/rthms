
#include <algorithm>
#include "rthms_allocator.h"
#include <assert.h>
#include <iomanip>

#define HLINE "======================================================================"

using namespace std;


// A sort and search algorihtm to find the maximum size of
// concurrent allocations in a list
double  Allocator::find_max_concurrent_alloc(vector<int>& objid_list){
    
    vector<ObjLifeSpan> time_events;
    
    vector<int>::iterator it;
    for (it=objid_list.begin(); it<objid_list.end(); ++it) {
        ObjLifeSpan a;
        a.time_stamp = objlist[*it].start_time;
        a.is_start   = true;
        a.obj_id     = *it;
        a.size_in_mb = objlist[*it].size_in_mb;
        time_events.push_back(a);
        
        ObjLifeSpan b;
        b.time_stamp = objlist[*it].end_time;
        b.is_start   = false;
        b.obj_id     = *it;
        b.size_in_mb = objlist[*it].size_in_mb;
        time_events.push_back(b);
    }
    
    sort(time_events.begin(), time_events.end(), sort_by_time);
    
    int    max_num_obj;
    int    curr_num_obj;
    double max_concurrent_mb  = 0.0;
    double curr_concurrent_mb = 0.0;
    vector<ObjLifeSpan>::iterator it2;
    for(it2=time_events.begin(); it2<time_events.end(); ++it2){
        if(it2->is_start){
            curr_num_obj ++;
            curr_concurrent_mb += it2->size_in_mb;
            if(curr_concurrent_mb > max_concurrent_mb)
                max_concurrent_mb = curr_concurrent_mb;
            
            if(curr_num_obj > max_num_obj)
                max_num_obj = curr_num_obj;
        }
        else{
            curr_num_obj --;
            curr_concurrent_mb -= it2->size_in_mb;
        }
    }

    return max_concurrent_mb;
}


bool    Allocator::merge_allocation(int memory_id, int src_id){

    vector<int> curr_obj_list = memory_allocation_list[memory_id].objid_list;
    
    curr_obj_list.insert( curr_obj_list.end(),
                         src_bid_list[src_id].objid_list.begin(),
                         src_bid_list[src_id].objid_list.end());
    
    double max_mb = find_max_concurrent_alloc(curr_obj_list);
    
    if(max_mb > memory_allocation_list[memory_id].capacity_in_mb )
        return false;
    
    else{
        memory_allocation_list[memory_id].srcid_list.push_back(src_id);
        memory_allocation_list[memory_id].objid_list.swap(curr_obj_list);
        return true;
    }
    
}


void    Allocator::build_src_bid(){
    
    int src_id = 0;
    map<string, vector<int> >::const_iterator it;
    for (it=src_obj_map.begin(); it!=src_obj_map.end(); ++it) {
        SrcBid a;
        a.src_id            = src_id++;
        a.src_string.append(it->first);
        a.objid_list        = it->second;
        a.max_concurrent_mb = find_max_concurrent_alloc(a.objid_list);
        a.curr_bid_id       = 0;
        
        //aggregate all the objects belonging to one src
        //only bid when feasible
        vector<MemoryAllocation>::iterator it2;
        for (it2=memory_allocation_list.begin(); it2<memory_allocation_list.end(); ++it2) {
            
            //check if the memory can hold the max concurrent allocation
            if(it2->capacity_in_mb < a.max_concurrent_mb) continue;
            
            //find the sum all objects' bidding
            Bid b;
            b.src_id    = a.src_id;
            b.memory_id = it2->memory_id;
            b.bidding   = 0.0;
            
            vector<int>::iterator it3;
            for (it3=a.objid_list.begin(); it3<a.objid_list.end(); ++it3) {
                b.bidding += objlist[*it3].preference_order[it2->memory_id];
            }
            
            a.bidding_list.push_back(b);
        }
        
        assert(a.bidding_list.size() > 0);
        
        src_bid_list.push_back(a);
    }
}


void    Allocator::allocate()
{

    build_src_bid();
    
    vector<Bid> bidding_pool;
    
    vector<SrcBid>::iterator it;
    for (it=src_bid_list.begin(); it<src_bid_list.end(); ++it){
        bidding_pool.push_back(it->bidding_list[it->curr_bid_id]);
    }
    
   
    while ( bidding_pool.size()>0 ) {
        
        sort(bidding_pool.begin(), bidding_pool.end(), sort_by_bidding);
        
        vector<Bid>::iterator it;
        for (it=bidding_pool.begin(); it<bidding_pool.end(); ++it) {
            bool can_merge = merge_allocation(it->memory_id, it->src_id);
            if( !can_merge){
                break;
            }else{
                src_bid_list[it->src_id].assigned_memory_id = it->memory_id;
            }
        }
        if( it<bidding_pool.end()){
            it --;
	    SrcBid &sb = src_bid_list[it->src_id];
            sb.curr_bid_id ++;
            *it = sb.bidding_list[sb.curr_bid_id];
            
            vector<Bid> new_bidding_pool(it, bidding_pool.end());
            bidding_pool = new_bidding_pool;
        }
    }

}

void Allocator::print_allocation(ostream& output){

    output    <<HLINE<<endl;
    output    <<"Allocation: "<<endl;
    output    <<HLINE<<endl;

    output << setw(64) << "Source Code" << setw(8) << "Memory";
    vector<SrcBid>::iterator it;
    for (it=src_bid_list.begin(); it<src_bid_list.end(); ++it){
        output << setw(20) << it->src_string << " " << it->assigned_memory_id;
    }
}

