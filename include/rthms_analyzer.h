#ifndef RTHMS_ANALYZER_H
#define RTHMS_ANALYZER_H

#include "rthms.h"
#include "rthms_config.h"
#include <map>

class Analyzer{
private:
    GlobalMetrics  totalmetrics;//the overall execution of the application
    std::vector<Memory> memory_list;//the list of available memories in a main system
    const int num_mem;
    std::vector<MetaObj> objlist;
    std::map<std::string, std::vector<int> > src_obj_map;
    
    void aggregate_thread_access(std::vector<MetaObj>::iterator it);
    void set_preference(std::vector<MetaObj>::iterator it);
    void analyze(std::vector<MetaObj>::iterator it);
    
public:
    Analyzer(GlobalMetrics& t_, const std::vector<Memory>& m_,std::vector<MetaObj>& l_)
      :num_mem(m_.size()){
      totalmetrics = t_;
      memory_list = m_;
      objlist = l_;
    }
    
    void analyze(int i);
    void analyzeAll();
    void print_preference(std::vector<MetaObj>::iterator obj, std::ostream& output=std::cout);
    void print_analysis(std::ostream& output=std::cout);
    void visualize(std::ostream& output=std::cout);
};

#endif

