#define OP_HOOK_OPEN        (1234)
#define OP_HOOK_CLOSE       (1235)
static inline void rthms_hook_open() {
  __asm__ __volatile__("" ::: "memory");
  __asm__ __volatile__("xchg %%rcx, %%rcx;" : : "c"(OP_HOOK_OPEN));
  __asm__ __volatile__("" ::: "memory");
}
static inline void rthms_hook_close() {
  __asm__ __volatile__("" ::: "memory");
  __asm__ __volatile__("xchg %%rcx, %%rcx;" : : "c"(OP_HOOK_CLOSE));
  __asm__ __volatile__("" ::: "memory");
}
