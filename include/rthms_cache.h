//Adapted from dcache.H in pin
#include <cstring>
#include <iomanip>
#include <cstdint>
#include <sstream>
#ifdef PIN
#include "rthms_config.h"
#else
#include "ar.h"
#endif

#define L1I_KB 32
#define L1D_KB 32
#define L1I_ASSOC 8
#define L1D_ASSOC 8
#define L2_KB    256
#define L2_ASSOC 8
#define L3_KB    1 //256 //16384
#define L3_ASSOC 8


#ifdef DRAMSIM
namespace ARCache{
  typedef uint64_t UINT64;
}
typedef ARCache::UINT64 CACHE_STATS; // type of cache hit/miss counters
#else
typedef uint64_t UINT64;
typedef UINT64 CACHE_STATS;
#endif

namespace CACHE_PROTOCOL
{
    typedef enum WRITE_POLICY{
      WRITE_BACK,
      WRITE_THROUGH
    }WRITE_POLICY;

    typedef enum WRITE_MISS_POLICY{
      WRITE_ALLOCATE,
      NO_WRITE_ALLOCATE
    }WRITE_MISS_POLICY;
}

typedef void     VOID;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int32_t  INT32;
typedef int64_t  INT64;
typedef uint64_t ADDRINT;
typedef char     CHAR;
typedef double   FLT64;
typedef float    FLT32;

using namespace std;
using namespace CACHE_PROTOCOL;

#ifndef PIN
inline string StringFlt(FLT64 val,UINT32 precision, UINT32 width){
  stringstream ss;
  ss << std::right<< std::fixed<<setw(width)<<setprecision(precision)<<val;
  return ss.str();
}
#define ASSERTX(expr) ( (expr) || printf("%s %s %i \n",#expr,__FILE__,__LINE__) )
inline string ljstr(const string& s, UINT32 width, CHAR padding = ' ')
{
  string  ostr(width,padding);
  ostr.replace(0,s.length(),s);
  return ostr;
}
inline string fltstr(FLT64 val, UINT32 prec=0,UINT32 width=0 ){
    return StringFlt(val,prec,width);
}
#endif

#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)


/*!
 *  @brief Checks if n is a power of 2.
 *  @returns true if n is power of 2
 */
static inline bool IsPower2(UINT32 n)
{
    return ((n & (n - 1)) == 0);
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 FloorLog2(UINT32 n)
{
    INT32 p = 0;
    
    if (n == 0) return -1;
    
    if (n & 0xffff0000) { p += 16; n >>= 16; }
    if (n & 0x0000ff00)	{ p +=  8; n >>=  8; }
    if (n & 0x000000f0) { p +=  4; n >>=  4; }
    if (n & 0x0000000c) { p +=  2; n >>=  2; }
    if (n & 0x00000002) { p +=  1; }
    
    return p;
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 CeilLog2(UINT32 n)
{
    return FloorLog2(n - 1) + 1;
}


/*!
 * Everything related to cache sets
 */
namespace CACHE_SET
{
    typedef struct CACHEBLOCK{
      int       valid;
      int       dirty;
      ADDRINT   tag;
      ADDRINT   addr;
      //int     memory;//for multi-memory
      ADDRINT   timestamp; //for LRU
    }CACHEBLOCK;
    
    typedef enum CACHELOOKUP{
      HIT_VALID,
      HIT_INVALID,
      MISS
    }CACHELOOKUP;
    
    template <UINT32 associativity=1,WRITE_POLICY write_policy=WRITE_BACK,WRITE_MISS_POLICY write_miss_policy=WRITE_ALLOCATE>
    class DIRECT_MAPPED
    {
    private:
      CACHEBLOCK   _block;
      WRITE_POLICY _write_policy;
      WRITE_MISS_POLICY _write_miss_policy;
      
    public:
      DIRECT_MAPPED()
	:_write_policy(write_policy),_write_miss_policy(write_miss_policy)
      {
	ASSERTX(associativity == 1); 
	_block.valid = 0;
	_block.dirty = 0;
	_block.tag = (ADDRINT)0;
      }
        
      UINT32 GetAssociativity() { return 1; }
        
      CACHELOOKUP  Find_and_Update(ADDRINT tag, bool isWrite)
      {
        bool found = (_block.tag == tag);

	if(found){
	  if(isWrite){ 
	    if( _write_policy==WRITE_BACK ){
	      _block.valid = 1;
	      _block.dirty = 1;
	    }else{ //writethrough always sync with main memory
	      _block.valid = 1;
	      _block.dirty = 0;
	    }
	    return HIT_VALID;
	  }else{//read needs to check valid bit
	    if(_block.valid==1) return HIT_VALID;
	    else return HIT_INVALID;
	  }
	}
	return MISS;
      }
      
      CACHEBLOCK  Evict_and_Update(ADDRINT tag, ADDRINT new_addr,bool isWrite)
      {
	CACHEBLOCK evict_block = _block;

	_block.valid = 1;
	_block.tag   = tag;
	_block.addr  = new_addr;
	if( isWrite && _write_policy==WRITE_BACK)
	  _block.dirty = 1;
	else
	  _block.dirty = 0;

	return evict_block;
      }

      vector<CACHEBLOCK> GetDirtyCacheline(){
        vector<CACHEBLOCK> addresses;
	if(_block.valid == 1 && _block.dirty == 1) addresses.push_back(_block);
	return addresses;
      }
    };
    
    /*!
     *  @brief Cache set with round robin replacement
     */
    template <UINT32 associativity,WRITE_POLICY write_policy,WRITE_MISS_POLICY write_miss_policy>    
    class ROUND_ROBIN
    {
    private:
    CACHEBLOCK _blocks[associativity];
    UINT32     _tagsLastIndex;
    UINT32     _nextReplaceIndex;
    const WRITE_POLICY _write_policy;
    const WRITE_MISS_POLICY _write_miss_policy;

    public:
    ROUND_ROBIN()
    :_write_policy(write_policy),_write_miss_policy(write_miss_policy)
    {
      _tagsLastIndex = associativity-1;
      _nextReplaceIndex = _tagsLastIndex;
      
      for (INT32 index = _tagsLastIndex; index >= 0; index--){
	_blocks[index].valid = 0;
	_blocks[index].dirty = 0;
	_blocks[index].tag = (ADDRINT)0;
      }
    }
    
    UINT32 GetAssociativity() { return _tagsLastIndex + 1; }        

    CACHELOOKUP Find_and_Update(ADDRINT tag, bool isWrite){
    
      INT32 found_index = -1;
      for(INT32 index = _tagsLastIndex;index>=0; index--)
	if(_blocks[index].tag == tag) found_index = index;
       
      if(found_index >=0 ){
	//Update if hit
	if(isWrite){ 
	  if( _write_policy==WRITE_BACK ){
	    _blocks[found_index].valid = 1;
	    _blocks[found_index].dirty = 1;
	  }else{ //writethrough always sync with main memory
	    _blocks[found_index].valid = 1;
	    _blocks[found_index].dirty = 0;
	  }
	  return HIT_VALID;
	}else{//read needs to check valid bit
	  if(_blocks[found_index].valid==1) return HIT_VALID;
	  else return HIT_INVALID;
	}
      }
      return MISS;
    }
    
    CACHEBLOCK  Evict_and_Update(ADDRINT tag,ADDRINT new_addr,bool isWrite){
      
      CACHE_SET::CACHEBLOCK evict_block = _blocks[_nextReplaceIndex];
      
      _blocks[_nextReplaceIndex].tag = tag;
      _blocks[_nextReplaceIndex].valid = 1;
      _blocks[_nextReplaceIndex].addr  = new_addr;
      
      if( isWrite && _write_policy==WRITE_BACK)
	_blocks[_nextReplaceIndex].dirty = 1;
      else
	_blocks[_nextReplaceIndex].dirty = 0;
     
      //update 
      _nextReplaceIndex = (_nextReplaceIndex == 0 ?_tagsLastIndex : (_nextReplaceIndex-1));
      return evict_block;
    }

    vector<CACHEBLOCK> GetDirtyCacheline(){
        vector<CACHEBLOCK> addresses;
	for(INT32 index = _tagsLastIndex; (index>=0); index--)
	  if(_blocks[index].valid == 1 && _blocks[index].dirty == 1) 
	    addresses.push_back(_blocks[index]);
	return addresses;
    }

    };
    
    
    template <UINT32 associativity,WRITE_POLICY write_policy,WRITE_MISS_POLICY write_miss_policy>
    class LRU
    {
    private:
        UINT32      _associativity;
        CACHEBLOCK  _blocks[associativity];
        UINT64      _timestamp;
        WRITE_POLICY _write_policy;
        WRITE_MISS_POLICY _write_miss_policy;

    public:
    LRU()
    :_associativity(associativity),_write_policy(write_policy),_write_miss_policy(write_miss_policy)
    {
	 for (UINT32 index = 0; index<_associativity; index++){
	      _blocks[index].valid = 0;
	      _blocks[index].dirty = 0;
	      _blocks[index].tag = (ADDRINT)0;
	 }
	 _timestamp=0;
    }

    UINT32 GetAssociativity() { return _associativity; }        
	    
    CACHELOOKUP Find_and_Update(ADDRINT tag, bool isWrite)
    {

#ifdef DEBUG_CACHE
	  for (UINT32 index = 0; index <_associativity; index++)
	    printf("LRU::block[%i].tag=%lu,valid=%i,timestamp=%lu\n",
		   index,_blocks[index].tag,_blocks[index].valid,_blocks[index].timestamp);
#endif

	  _timestamp ++;
	  
	  INT32 found_index = -1;
	  for (UINT32 index = 0;index<_associativity; index++)
	    if(_blocks[index].tag==tag) found_index=index;
	  
	  if(found_index>-1){
	    _blocks[found_index].timestamp = _timestamp;
	    if(isWrite){
	      if(_write_policy==WRITE_BACK ){
		_blocks[found_index].valid = 1;
		_blocks[found_index].dirty = 1;
	      }else{ //writethrough always sync with main memory
		_blocks[found_index].valid = 1;
		_blocks[found_index].dirty = 0;
	      }
	      return HIT_VALID;
	    }else{//read needs to check valid bit
	      if(_blocks[found_index].valid==1) return HIT_VALID;
	      else return HIT_INVALID;
	    }
	  }
	  return MISS;
    }

    ///boolisWriteBack,size_t memory, ADDRINT time_stamp
    CACHEBLOCK Evict_and_Update(ADDRINT tag, ADDRINT new_addr, bool isWrite)
    {

      //Prioirty: invalid > oldest
      UINT32 evict_index = 0;
      bool found = false;
      for (UINT32 index = 0; (index<_associativity) && !found; index++)
	{
	  if(_blocks[index].valid == 0){
	    evict_index = index;
	    found = true;
	  }
	}

      //Find the least recently used block
      if(!found){
	evict_index = 0;
	UINT32 min_time  = _blocks[0].timestamp;
	for (UINT32 index = 1; index <_associativity; index++)
	  {
	    if( _blocks[index].timestamp<min_time){
	      min_time   = _blocks[index].timestamp;
	      evict_index= index;
	    }
	  }
      }
            
      CACHEBLOCK evict_block = _blocks[evict_index];

      //update cache block
      _blocks[evict_index].valid= 1;
      _blocks[evict_index].tag  = tag;
      if( isWrite && _write_policy==WRITE_BACK)
	_blocks[evict_index].dirty= 1;
      else
	_blocks[evict_index].dirty= 0;
      _blocks[evict_index].addr   = new_addr;
      _blocks[evict_index].timestamp = _timestamp;

#ifdef DEBUG_CACHE
      for (UINT32 index = 0; index <_associativity; index++)
	printf("Evict_and_Update block[%i]: tag=%lu, valid=%i,dirty=%i,memory=%i,addr=%lx,timestamp=%lu\n",
	       index, _blocks[index].tag,_blocks[index].valid,_blocks[index].dirty,
	       _blocks[index].addr,_blocks[index].timestamp);
#endif
      return evict_block;
    }

    vector<CACHEBLOCK> GetDirtyCacheline(){
      vector<CACHEBLOCK> addresses;
      for(INT32 index = 0; index<_associativity; index++)
	if(_blocks[index].valid == 1 && _blocks[index].dirty == 1) 
	  addresses.push_back(_blocks[index]);
      return addresses;
    }
 };
   
} // namespace CACHE_SET



class CACHE_BASE
{
public:
    // types, constants
    typedef enum
    {
        ACCESS_TYPE_LOAD,
        ACCESS_TYPE_STORE,
        ACCESS_TYPE_NUM
    } ACCESS_TYPE;
    
    typedef enum
    {
        CACHE_TYPE_ICACHE,
        CACHE_TYPE_DCACHE,
        CACHE_TYPE_NUM
    } CACHE_TYPE;
    
protected:
    static const UINT32 HIT_MISS_NUM = 2;
    CACHE_STATS _access[ACCESS_TYPE_NUM][HIT_MISS_NUM];
    
    // input params
    const std::string _name;
    const UINT32 _cacheSize;
    const UINT32 _lineSize;
    const UINT32 _associativity;
    
    // computed params
    const UINT32 _lineShift;
    const UINT32 _num_sets;
    const UINT32 _setIndexMask;


private:
    CACHE_STATS SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;        
        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
        {
            sum += _access[accessType][hit];
        }
        
        return sum;
    }
        
public:
    // constructors/destructors
    CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity);
    
    // accessors
    UINT32 CacheSize() const { return _cacheSize; }
    UINT32 LineSize() const { return _lineSize; }
    UINT32 Associativity() const { return _associativity; }
    //
    CACHE_STATS Hits(ACCESS_TYPE accessType) const { return _access[accessType][true];}
    CACHE_STATS Misses(ACCESS_TYPE accessType) const { return _access[accessType][false];}
    CACHE_STATS Accesses(ACCESS_TYPE accessType) const { return Hits(accessType) + Misses(accessType);}
    CACHE_STATS Hits() const { return SumAccess(true);}
    CACHE_STATS Misses() const { return SumAccess(false);}
    CACHE_STATS Accesses() const { return Hits() + Misses();}
    
    VOID SplitAddress(const ADDRINT addr, ADDRINT & tag, UINT32 & setIndex) const
    {
        tag = addr >> _lineShift;
        setIndex = tag & _setIndexMask;
    }
    
    VOID SplitAddress(const ADDRINT addr, ADDRINT & tag, UINT32 & setIndex, UINT32 & lineIndex) const
    {
        const UINT32 lineMask = _lineSize - 1;
        lineIndex = addr & lineMask;
        SplitAddress(addr, tag, setIndex);
    }
    
    string StatsLong(string prefix = "", CACHE_TYPE = CACHE_TYPE_DCACHE) const;
};


template <class SET,UINT32 associativity,UINT32 cacheSize,UINT32 lineSize,WRITE_POLICY write_policy,WRITE_MISS_POLICY write_miss_policy> class CACHE : public CACHE_BASE
{
 private:
    SET *_sets;
    const bool _isWriteThrough;
    const bool _isWriteAllocate;
 public:

 CACHE(std::string name)
   :CACHE_BASE(name, cacheSize, lineSize, associativity),
      _isWriteThrough(write_policy==WRITE_THROUGH),
      _isWriteAllocate(write_miss_policy==WRITE_ALLOCATE){  
      _sets = new SET[_num_sets];
    }

    ~CACHE(){delete _sets;}
    
    bool get_isWriteThrough(){ return _isWriteThrough;}
    bool get_isWriteAllocate(){return _isWriteAllocate;}

    CACHE_SET::CACHEBLOCK lookup_evict_update(ADDRINT addr,ACCESS_TYPE accessType)
    {
      ADDRINT tag      = addr >> _lineShift;
      UINT32 setIndex  = tag & _setIndexMask;
      SET &set         = _sets[setIndex];
      bool isWrite     = (accessType==ACCESS_TYPE_STORE);
      bool isRead      = !isWrite;

      //Step 1: Hit or Miss
      CACHE_SET::CACHELOOKUP lookup = set.Find_and_Update(tag,isWrite);
      bool hit = (lookup==CACHE_SET::HIT_VALID || lookup==CACHE_SET::HIT_INVALID);
      _access[accessType][hit]++; //for statistics
      
#ifdef DEBUG_CACHE
      printf("Find_and_Update: addr%lx  setIndex %i tag %lu %s\n",
	     addr, setIndex, tag, (hit ?"hit" : "miss"));
#endif

      //Step 2: evict if read miss or write miss with write_allocate policy
      CACHE_SET::CACHEBLOCK evict_block;
      if(lookup==CACHE_SET::MISS){

	evict_block.dirty=0; //init dirty bit
	if(isRead || _isWriteAllocate) 
	  evict_block = set.Evict_and_Update(tag,addr,isWrite);

	evict_block.tag = 0; // 0: miss; 1: hit 

      }else{
	evict_block.tag = 1; // 0: miss; 1: hit 

        if(lookup==CACHE_SET::HIT_VALID) evict_block.valid = 1;
	else evict_block.valid = 0;
      }

#ifdef DEBUG_CACHE
      printf("evict_block: tag%lu, valid%i, dirty%i, memory%i, addr=%lx\n",
	     evict_block.tag,evict_block.valid,evict_block.dirty, evict_block.addr);
#endif

    return evict_block;
 }

 

 vector<CACHE_SET::CACHEBLOCK> flush_ditry_cacheline(){
   vector<CACHE_SET::CACHEBLOCK> addresses;
   for (UINT32 i = 0; i < _num_sets; i++)
     {
       vector<CACHE_SET::CACHEBLOCK> tmp = _sets[i].GetDirtyCacheline();
       addresses.insert(addresses.end(), tmp.begin(), tmp.end());
     }
   return addresses;
 }
    };


/* define shortcuts
#define CACHE_DIRECT_MAPPED(SETS,WRITE_POLICY,WRITE_MISS_POLICY) CACHE<CACHE_SET::DIRECT_MAPPED,SETS,WRITE_POLICY,WRITE_MISS_POLICY>
#define CACHE_ROUND_ROBIN(SETS,ASSOCIATIVITY,WRITE_POLICY,WRITE_MISS_POLICY) CACHE<CACHE_SET::ROUND_ROBIN<ASSOCIATIVITY>,SETS,WRITE_POLICY,WRITE_MISS_POLICY>
#define CACHE_LRU(SETS,ASSOCIATIVITY,WRITE_POLICY,WRITE_MISS_POLICY) CACHE<CACHE_SET::LRU<ASSOCIATIVITY>,SETS,WRITE_POLICY,WRITE_MISS_POLICY>


namespace L1D
{
    const UINT32 cacheSize = L1D_KB*KILO;
    const UINT32 lineSize  = CACHELINE_BYTES;
    const UINT32 associativity = L1D_ASSOC;
    const UINT32 num_sets = cacheSize / (lineSize * associativity);

    //typically, write_through+no_write_allocate or write_back + write_allocate
    const WRITE_POLICY write_policy = WRITE_BACK;
    const WRITE_MISS_POLICY write_miss_policy = WRITE_ALLOCATE;
    
    typedef CACHE_LRU(num_sets,associativity,write_policy,write_miss_policy) CACHE;
}

namespace L1I
{
    const UINT32 cacheSize = L1I_KB*KILO;
    const UINT32 lineSize  = CACHELINE_BYTES;
    const UINT32 associativity = L1I_ASSOC;
    const UINT32 num_sets = cacheSize / (lineSize * associativity);

    //typically, write_through+no_write_allocate or write_back + write_allocate
    const WRITE_POLICY write_policy = WRITE_BACK;
    const WRITE_MISS_POLICY write_miss_policy = WRITE_ALLOCATE;
    
    typedef CACHE_LRU(num_sets,associativity,write_policy,write_miss_policy) CACHE;
}

namespace L2
{
    const UINT32 cacheSize = L2_KB*KILO;
    const UINT32 lineSize  = CACHELINE_BYTES;
    const UINT32 associativity = L2_ASSOC;
    const UINT32 num_sets = cacheSize / (lineSize * associativity);

    //typically, write_through+no_write_allocate or write_back + write_allocate
    const WRITE_POLICY write_policy = WRITE_BACK;
    const WRITE_MISS_POLICY write_miss_policy = WRITE_ALLOCATE;

    typedef CACHE_LRU(num_sets,associativity,write_policy,write_miss_policy) CACHE;
}*/

namespace L3
{
    const UINT32 cacheSize = L3_KB*KILO;
    const UINT32 lineSize  = CACHELINE_BYTES;
    const UINT32 associativity = L3_ASSOC;

    //typically, write_through+no_write_allocate or write_back + write_allocate
    const WRITE_POLICY wp = WRITE_BACK;
    const WRITE_MISS_POLICY wmp = WRITE_ALLOCATE;
    //const WRITE_POLICY wp = WRITE_THROUGH;
    //const WRITE_MISS_POLICY wmp = NO_WRITE_ALLOCATE;

    typedef CACHE_SET::LRU<associativity,wp,wmp> SET;
    typedef CACHE<SET,associativity,cacheSize,lineSize,wp,wmp> CACHE;
}


