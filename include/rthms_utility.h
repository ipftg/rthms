#ifndef RTHMS_UTILITY_H
#define RTHMS_UTILITY_H

#include "rthms.h"
#include "pin.H"

void get_stack_range();

void setup_trackfunclist(std::vector<std::string> &trackfunclist, std::string& filename);

void setup_allocfunclist(std::vector<AllocFunc> &allocfunclist, std::string& filename);
void setup_allocfunclist(std::vector<AllocFunc> &allocfunclist);
void setup_allocfunclist_internal(std::vector<AllocFunc> &allocfunclist);

VOID setup_freefunclist(std::vector<AllocFunc> &freefunclist, std::string& filename);

void setup_memorylist(std::vector<Memory> &memory_list, std::string& filename);

void setup_arithmeticinstruction(std::vector<OPCODE> &fpop_list, std::string& filename);

size_t  setup_hooks(std::vector<string> &hooklist, std::string& filename);

void get_var_name(ADDRINT returnip);
#endif
