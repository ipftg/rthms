string var(sym_name);
if(var.compare("noautom_")==0){
  cout << "NPB MG: common /noautom/ u,v,r " << endl;
  cout << "NPB MG: double precision u(nr),v(nv),r(nr)" << endl;
  
#ifdef CLASS_S
  const int nr = 46480;
  const int nv = 39304;
#endif
#ifdef CLASS_A
  const int nr = 19704488;
  const int nv = 17173512;
#endif
#ifdef CLASS_B
  const int nr = 19704488;
  const int nv = 17173512;
#endif
#ifdef CLASS_C
  const int nr = 155501232;
  const int nv = 135796744;
#endif


  uint64_t addr= base_addr+symbols[s].st_value;
  uint64_t addr_u = addr;
  uint64_t addr_v = addr_u + sizeof(double)*nr;
  uint64_t addr_r = addr_v + sizeof(double)*nv;

	MetaObj tempObj;
	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("u");
	tempObj.st_addr = addr_u;
	tempObj.end_addr= addr_v-1;
	tempObj.size    = sizeof(double)*nr;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	MetaObj& a = masterlist.back();
	TraceObjMeta b( a );
	activelist.push_back(b);


	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.clear();
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("v");
	tempObj.st_addr = addr_v;
	tempObj.end_addr= addr_r-1;
	tempObj.size    = sizeof(double)*nv;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta b1( masterlist.back());
	activelist.push_back(b1);


	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.clear();
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("r");
	tempObj.st_addr = addr_r;
	tempObj.size    = sizeof(double)*nr;
	tempObj.end_addr= addr_r+tempObj.size-1;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta b2( masterlist.back());
	activelist.push_back(b2);
 
}else if(var.compare("bigarrays_")==0){
  cout << "NPB FT: common /bigarrays/ u0, pad1, u1, pad2, twiddle " << endl;
  cout << "NPB FT: double complex   u0(ntotalp),u1(ntotalp),pad1(3), pad2(3)" << endl;
  cout << "NPB FT: double precision twiddle(ntotalp)" << endl;
  
#ifdef CLASS_S
  const int ntotalp = 266240;
#endif
#ifdef CLASS_A
  const int ntotalp = 8421376;
#endif
#ifdef CLASS_B
  const int ntotalp = 33619968;
#endif
#ifdef CLASS_C
  const int ntotalp = 155501232;
#endif

  uint64_t addr= base_addr+symbols[s].st_value;
  uint64_t addr_u0 = addr;
  uint64_t addr_u1 = addr_u0 + sizeof(double)*2*(ntotalp+3);
  uint64_t addr_twiddle = addr_u1 + sizeof(double)*2*(ntotalp+3);

	MetaObj tempObj;
	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("u0");
	tempObj.st_addr = addr_u0;
	tempObj.end_addr= addr_u0+ sizeof(double)*2*ntotalp -1;
	tempObj.size    = tempObj.end_addr- tempObj.st_addr +1;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	MetaObj& a = masterlist.back();
	TraceObjMeta b( a );
	activelist.push_back(b);


	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.clear();
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("u1");
	tempObj.st_addr = addr_u1;
	tempObj.end_addr= addr_u1+sizeof(double)*2*ntotalp -1;
	tempObj.size    = tempObj.end_addr - tempObj.st_addr+1;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta b1( masterlist.back());
	activelist.push_back(b1);


	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.clear();
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("twiddle");
	tempObj.st_addr = addr_twiddle;
	tempObj.size    = sizeof(double)*ntotalp;
	tempObj.end_addr= tempObj.st_addr+tempObj.size-1;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta b2( masterlist.back());
	activelist.push_back(b2);
 
}
else if(var.compare("main_flt_mem_")==0){
  cout << "NPB CG: common /main_flt_mem/v, aelt, a, x, z, p, q, r" << endl;
  cout << "NPB CG: double precision v(nz), aelt(naz), a(nz), x(na+2), z(na+2), p(na+2), q(na+2), r(na+2)" << endl;

  #ifdef CLASS_S
  const int nz = 89600;
  const int naz= 11200;
  const int na = 1400;
  #endif
  #ifdef CLASS_A
  const int nz = 2016000;
  const int naz= 168000;
  const int na = 14000;
  #endif
  #ifdef CLASS_B
  const int nz = 14700000;
  const int naz= 1050000;
  const int na = 75000;
  #endif
  #ifdef CLASS_C
  const int nz = 38400000;
  const int naz= 2400000;
  const int na = 150000;
  #endif

  uint64_t addr= base_addr+symbols[s].st_value;
  uint64_t addr_v = addr;
  uint64_t addr_aelt = addr_v + sizeof(double)*nz;
  uint64_t addr_a = addr_aelt + sizeof(double)*naz;
  uint64_t addr_x = addr_a + sizeof(double)*nz;
  uint64_t addr_z = addr_x + sizeof(double)*(na+2);
  uint64_t addr_p = addr_z + sizeof(double)*(na+2);
  uint64_t addr_q = addr_p + sizeof(double)*(na+2);
  uint64_t addr_r = addr_q + sizeof(double)*(na+2);

	MetaObj tempObj;
	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("v");
	tempObj.st_addr = addr_v;
	tempObj.end_addr= addr_aelt-1;
	tempObj.size    = sizeof(double)*nz;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta v( masterlist.back() );
	activelist.push_back(v);


	tempObj.var_name.clear();
	tempObj.var_name.append("aelt");
	tempObj.st_addr = addr_aelt;
	tempObj.end_addr= addr_a-1;
	tempObj.size    = sizeof(double)*naz;
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta aelt(masterlist.back());
	activelist.push_back(aelt);


	tempObj.var_name.clear();
	tempObj.var_name.append("a");
	tempObj.st_addr = addr_a;
	tempObj.end_addr= addr_x-1;
	tempObj.size    = sizeof(double)*nz;
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta a(masterlist.back());
	activelist.push_back(a);


	tempObj.var_name.clear();
	tempObj.var_name.append("x");
	tempObj.st_addr = addr_x;
	tempObj.end_addr= addr_z-1;
	tempObj.size    = sizeof(double)*(na+2);
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta x(masterlist.back());
	activelist.push_back(x);

	tempObj.var_name.clear();
	tempObj.var_name.append("z");
	tempObj.st_addr = addr_z;
	tempObj.end_addr= addr_p-1;
	tempObj.size    = sizeof(double)*(na+2);
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta z(masterlist.back());
	activelist.push_back(z);

	tempObj.var_name.clear();
	tempObj.var_name.append("p");
	tempObj.st_addr = addr_p;
	tempObj.end_addr= addr_q-1;
	tempObj.size    = sizeof(double)*(na+2);
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta p(masterlist.back());
	activelist.push_back(p);


	tempObj.var_name.clear();
	tempObj.var_name.append("q");
	tempObj.st_addr = addr_q;
	tempObj.end_addr= addr_r-1;
	tempObj.size    = sizeof(double)*(na+2);
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta q(masterlist.back());
	activelist.push_back(q);


	tempObj.var_name.clear();
	tempObj.var_name.append("r");
	tempObj.st_addr = addr_r;
	tempObj.size    = sizeof(double)*(na+2);
	tempObj.end_addr= addr_r + tempObj.size -1;
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta r(masterlist.back());
	activelist.push_back(r);

 }else if(var.compare("main_int_mem_")==0){
  cout << "NPB CG: common / main_int_mem /colidx,rowstr,iv, arow, acol" << endl;
  cout << "NPB CG: integer colidx(nz),rowstr(na+1),iv(nz+na),arow(na),acol(naz)" << endl;
  
  #ifdef CLASS_S
  const int nz = 89600;
  const int naz= 11200;
  const int na = 1400;
  #endif
  #ifdef CLASS_A
  const int nz = 2016000;
  const int naz= 168000;
  const int na = 14000;
  #endif
  #ifdef CLASS_B
  const int nz = 14700000;
  const int naz= 1050000;
  const int na = 75000;
  #endif
  #ifdef CLASS_C
  const int nz = 38400000;
  const int naz= 2400000;
  const int na = 150000;
  #endif

  uint64_t addr= base_addr+symbols[s].st_value;
  uint64_t addr_colidx = addr;
  uint64_t addr_rowstr = addr_colidx + sizeof(int)*nz;
  uint64_t addr_iv   = addr_rowstr + sizeof(int)*(na+1);
  uint64_t addr_arow = addr_iv + sizeof(int)*(nz+na);
  uint64_t addr_acol = addr_arow + sizeof(int)*na;
  
	MetaObj tempObj;
	tempObj.type = AllocType::STATIC_T;
	tempObj.creator_tid = 0;
	tempObj.source_code_info.append("Image");
	tempObj.var_name.clear();
	tempObj.var_name.append("colidx");
	tempObj.st_addr = addr_colidx;
	tempObj.end_addr= addr_rowstr-1;
	tempObj.size    = sizeof(int)*nz;
	tempObj.obj_id  = masterlist.size();
	tempObj.st_ins  = 0;
	tempObj.st_memins  = 0;
	tempObj.num_threads= 0;
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta colidx( masterlist.back() );
	activelist.push_back(colidx);


	tempObj.var_name.clear();
	tempObj.var_name.append("rowstr");
	tempObj.st_addr = addr_rowstr;
	tempObj.end_addr= addr_iv-1;
	tempObj.size    = sizeof(int)*(na+1);
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta rowstr(masterlist.back());
	activelist.push_back(rowstr);


	tempObj.var_name.clear();
	tempObj.var_name.append("iv");
	tempObj.st_addr = addr_iv;
	tempObj.end_addr= addr_arow-1;
	tempObj.size    = sizeof(int)*(nz+na);
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta iv(masterlist.back());
	activelist.push_back(iv);


	tempObj.var_name.clear();
	tempObj.var_name.append("arow");
	tempObj.st_addr = addr_arow;
	tempObj.end_addr= addr_acol-1;
	tempObj.size    = sizeof(int)*na;
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta arow(masterlist.back());
	activelist.push_back(arow);


	tempObj.var_name.clear();
	tempObj.var_name.append("acol");
	tempObj.st_addr = addr_acol;
	tempObj.size    = sizeof(int)*naz;
	tempObj.end_addr= addr_acol + tempObj.size -1;
	tempObj.obj_id  = masterlist.size();
	masterlist.push_back(tempObj);
#ifdef DEBUG
	masterlist.back().print_meta();
#endif
	//add the obj to active list
	TraceObjMeta acol(masterlist.back());
	activelist.push_back(acol);

  }

