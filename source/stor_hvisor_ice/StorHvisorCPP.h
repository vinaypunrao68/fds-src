
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif
BEGIN_C_DECLS
void init_DPAPI();
void integration_stub( void *buf,  int len);
END_C_DECLS
