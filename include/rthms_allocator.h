#ifndef RTHMS_ALLOCATOR_H
#define RTHMS_ALLOCATOR_H

#include "rthms.h"
#include "rthms_analyzer.h"

typedef struct MemoryAllocation{
    int memory_id;
    int capacity_in_mb;
  std::vector<int> srcid_list;
  std::vector<int> objid_list;
}MemoryAllocation;


typedef struct Bid{
    int src_id;
    int memory_id;
    double bidding;
}Bid;

//sort the bidding in descending order
bool sort_by_bidding(const Bid& lhs, const Bid& rhs){
    return lhs.bidding >= rhs.bidding;
}


typedef struct SrcBid{
    int     src_id;
    std::string  src_string;
    std::vector<int> objid_list;
    double  max_concurrent_mb;
    std::vector<Bid>  bidding_list;
    int     curr_bid_id;
    int     assigned_memory_id;
}SrcBid;


//The following struct will be used for sorting to find
//the max size of concurrent allocation
typedef struct ObjLifeSpan{
    double time_stamp;
    bool   is_start; //either start or end
    int    obj_id;
    double size_in_mb;
}ObjLifeSpan;


//sort the life span of memory objects
bool sort_by_time(const ObjLifeSpan& lhs, const ObjLifeSpan& rhs){
    return lhs.time_stamp <= rhs.time_stamp;
}


class Allocator{
private:
    //the list of available memories in a main system
    std::vector<MemoryAllocation> memory_allocation_list;
    const int num_mem;
    const std::vector<MetaObj> objlist;
    const std::map<std::string, std::vector<int> > src_obj_map;
    std::vector<SrcBid> src_bid_list;
    

    double  find_max_concurrent_alloc(std::vector<int>& objid_list);
    bool    merge_allocation(int memory_id, int src_id);
    void    build_src_bid();
    
public:
    Allocator(std::vector<Memory>& m_, std::vector<MetaObj>& l_, std::map<std::string, std::vector<int> >& s_)
      :num_mem(m_.size()),
      objlist(l_),
      src_obj_map(s_)
      {
	std::vector<Memory>::iterator it;
        for (it=m_.begin(); it<m_.end(); ++it){
            MemoryAllocation a;
            a.memory_id = it->memory_id;
            a.capacity_in_mb = it->capacity_in_mb;
            memory_allocation_list.push_back(a);
        }
    }
    
    void allocate();
    
    void print_allocation(std::ostream& output=std::cout);
};

#endif

