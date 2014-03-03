#ifndef __ELEV8_H__
#define __ELEV8_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Elev8_Context Elev8_Context;

void		elev8_init();
void		elev8_shutdown();

Elev8_Context	*elev8_context_add();
void		elev8_context_del(Elev8_Context *ctx);
void		elev8_context_enter(Elev8_Context *ctx);
void		elev8_context_leave(Elev8_Context *ctx);

Eina_Bool	elev8_context_script_exec(Elev8_Context *ctx, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* __ELEV8_H__ */