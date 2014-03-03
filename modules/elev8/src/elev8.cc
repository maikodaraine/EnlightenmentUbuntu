#include <dlfcn.h>
#include <Ecore_Con.h>
#include <Ecore.h>
#include <Eina.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <v8.h>

#include "elev8.h"
#include "timer.h"
#include "storage.h"
#include "environment.h"
#include "utils.h"
#include "gadcon.h"

#define MODLOAD_ENV "ELEV8_MODPATH"
#define MODLOAD_ENV_DEFAULT_DIRS ".:"

#define CRIT(...) do { fputs("CRIT: ", stderr); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while(0)

using namespace v8;

enum ContextUseRule{
   CREATE_NEW_CONTEXT,
   USE_CURRENT_CONTEXT
};

struct _Elev8_Context {
    Persistent<Value> v8_self;
    Persistent<ObjectTemplate> global;
    Persistent<Context> context;
    Persistent<Object> module_cache;
};

static int log_domain;
static Ecore_Event_Handler *siguser_handler;

static Handle<Value>
_string_to_object(Handle<String> str)
{
   HandleScope scope;

   Handle<Value> json = Context::GetCurrent()->Global()->Get(String::NewSymbol("JSON"));
   if (json.IsEmpty())
     return Null();

   Handle<Value> stringify = json->ToObject()->Get(String::NewSymbol("parse"));
   if (stringify.IsEmpty())
     return Null();

   Handle<Function> func = Handle<Function>::Cast(stringify);
   Handle<Value> args[1] = { str };

   return scope.Close(func->Call(Context::GetCurrent()->Global(), 1, args)->ToObject());
}

static int
_shebang_length(const char *p, int len)
{
   if ((len >= 2) && (p[0] == '#') && (p[1] == '!'))
     return (const char *)memchr(&p[2], '\n', len) - p;
   return 0;
}

Handle<String>
_load_script_from_file(const char *filename)
{
   HandleScope scope;
   Handle<String> ret;
   int fd, len = 0;
   char *p;
   int n;

   fd = open(filename, O_RDONLY);
   if (fd < 0) goto fail_open;

   len = lseek(fd, 0, SEEK_END);
   if (len <= 0) goto fail;

   p = reinterpret_cast<char*>(mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0));
   if (p == reinterpret_cast<char *>(MAP_FAILED)) goto fail;

   n = _shebang_length(p, len);
   ret = String::New(&p[n], len - n);

   munmap(p, len);

fail:
   close(fd);

fail_open:
   return scope.Close(ret);
}

static char *
_find_module_file_name(char *module_name, const char *prefix, const char *type)
{
   char *modpath = getenv(MODLOAD_ENV);
   char default_modpath[] = MODLOAD_ENV_DEFAULT_DIRS;

   if (!modpath) modpath = default_modpath;

   for (char *token, *rest, *ptr = modpath;
             (token = strtok_r(ptr, ":", &rest));
             ptr = rest)
      {
         char full_path[PATH_MAX];

         if (snprintf(full_path, PATH_MAX - 1, "%s/%s%s.%s", token, prefix, module_name, type) < 0)
             return NULL;

         if (!access(full_path, R_OK))
             return strdup(full_path);
      }

   return NULL;
}

inline static char *
_find_native_module_file_name(char *module_name)
{
   return _find_module_file_name(module_name, "lib", "so");
}

inline static char *
_find_js_module_file_name(char *module_name)
{
   char *tmp;
   char *path = _find_module_file_name(module_name, "", "js");

   if (path)
     return path;

   if (asprintf(&tmp, "%s/", module_name) < 0)
     goto end;

   path = _find_module_file_name((char *)"package", tmp, "json");
   free(tmp);

   if (path)
     {
        HandleScope scope;
        Handle<String> package_json = _load_script_from_file(path);
        Handle<Value> package_obj = _string_to_object(package_json);

        if (!package_obj->IsObject())
          goto end;

        Handle<Value> main_module = package_obj->ToObject()->Get(String::NewSymbol("main"));
        if (!main_module->IsString())
          goto end;

        if (asprintf(&tmp, "%s/%s", dirname(path),
                    *String::Utf8Value(main_module->ToString())) > 0)
          {
             free(path);
             return !access(tmp, R_OK) ? tmp : NULL;
          }
   }

end:
   free(path);
   return NULL;
}

static bool
_module_native_load(Elev8_Context *, char *module_name, Handle<Object> name_space, ContextUseRule)
{
   char *file_name = _find_native_module_file_name(module_name);

   if (!file_name) return false;

   // FIXME: Use Eina_Module here.
   void *handle = dlopen(file_name, RTLD_LAZY);
   if (!handle)
     {
        CRIT("Could not dlopen(%s): %s", file_name, dlerror());
        free(file_name);
        return false;
     }

   void (*init_func)(Handle<Object> name_space);
   init_func = (void (*)(Handle<Object>))dlsym(handle, "RegisterModule");
   if (!init_func)
     {
        CRIT("Could not dlsym(%p, RegisterModule): %s", handle, dlerror());
        free(file_name);
        dlclose(handle);
        return false;
     }

   init_func(name_space);
   free(file_name);
   return true;
}

static bool
_module_js_load(Elev8_Context *ctx, char *module_name, Handle<Object> name_space, ContextUseRule context_use_rule)
{
   char *file_name = _find_js_module_file_name(module_name);
   bool return_value = false;

   if (!file_name) return false;

   Handle<String> mod_source = _load_script_from_file(file_name);
   if (mod_source.IsEmpty())
     {
        free(file_name);
        return false;
     }

   HandleScope handle_scope;

   Persistent<Context> mod_context;
   if (context_use_rule == CREATE_NEW_CONTEXT)
     {
        ctx->global->Set(String::NewSymbol("exports"), name_space);
        mod_context = Context::New(NULL, ctx->global);
        mod_context->Enter();

        //run_script(PACKAGE_LIB_DIR "/../init.js");
     }

   TryCatch try_catch;
   Local<Script> mod_script = Script::Compile(mod_source->ToString(), String::New(file_name));
   if (try_catch.HasCaught())
     {
//        boom(try_catch);
        goto end;
     }

   mod_script->Run();
   if (try_catch.HasCaught())
     {
//        boom(try_catch);
        goto end;
     }

   return_value = true;
end:
   free(file_name);

   if (context_use_rule == CREATE_NEW_CONTEXT)
     mod_context->Exit();

   return return_value;
}

static bool
_load_module_with_type_hints(Elev8_Context *ctx, Handle<String> module_name, Local<Object> name_space, ContextUseRule context_use_rule)
{
   String::Utf8Value module_name_utf(module_name);

   if (module_name->Length() <= 3) goto end;

   if (eina_str_has_suffix(*module_name_utf, ".js"))
     {
        *(*module_name_utf + module_name->Length() - 3) = '\0';
        return _module_js_load(ctx, *module_name_utf, name_space, context_use_rule);
     }

   if (eina_str_has_suffix(*module_name_utf, ".so"))
     {
        *(*module_name_utf + module_name->Length() - 3) = '\0';
        return _module_native_load(ctx, *module_name_utf, name_space, context_use_rule);
     }

end:
   return _module_native_load(ctx, *module_name_utf, name_space, context_use_rule)
       || _module_js_load(ctx, *module_name_utf, name_space, context_use_rule);
}

static Handle<Value>
_load_module(Elev8_Context *ctx, Handle<String> module_name, ContextUseRule context_use_rule)
{
   HandleScope scope;

   if (ctx->module_cache->HasOwnProperty(module_name))
     return scope.Close(ctx->module_cache->Get(module_name));

   Local<Object> name_space = (context_use_rule == CREATE_NEW_CONTEXT) ?
                Object::New() : ctx->context->Global();

   if (_load_module_with_type_hints(ctx, module_name, name_space, context_use_rule))
     {
        ctx->module_cache->Set(module_name, Persistent<Object>::New(name_space));
        return scope.Close(name_space);
     }

   Local<String> msg = String::Concat(String::New("Cannot load module: "), module_name);
   return scope.Close(ThrowException(Exception::Error(msg)));
}

static inline Handle<Value>
_internal_require(Elev8_Context *ctx, const Arguments& args, ContextUseRule cur)
{
   HandleScope scope;
   if (args.Length() < 1)
     return scope.Close(ThrowException(Exception::Error(String::New("Module name missing"))));
   return scope.Close(_load_module(ctx, args[0]->ToString(), cur));
}

static void
_message_handler(Handle<Message> message, Handle<Value>)
{
    CRIT(*String::Utf8Value(message->Get()));
}

static Eina_Bool
_flush_garbage_collector(void *, int, void *)
{
    V8::LowMemoryNotification();
    return ECORE_CALLBACK_CANCEL;
}

static Handle<Value>
_require(const Arguments& args)
{
    Elev8_Context *ctx = (Elev8_Context *)External::Cast(*args.Data())->Value();
    return _internal_require(ctx, args, CREATE_NEW_CONTEXT);
}

static Handle<Value>
___require__(const Arguments& args)
{
    Elev8_Context *ctx = (Elev8_Context *)External::Cast(*args.Data())->Value();
    return _internal_require(ctx, args, USE_CURRENT_CONTEXT);
}

static Handle<Value>
_modules(const Arguments& args)
{
    Elev8_Context *ctx = (Elev8_Context *)External::Cast(*args.Data())->Value();
    return ctx->module_cache;
}

static Handle<Value>
_print(const Arguments& args)
{
    HandleScope scope;
    int argument_count = args.Length();

    if (!argument_count) goto end;

    for (int i = 0; i < argument_count; i++)
      {
         fputs(*String::Utf8Value(args[i]), stdout);

         if (i < argument_count - 1)
           putchar(' ');
      }

end:
    putchar('\n');
    fflush(stdout);

    return Undefined();
}

static void
_register_basic_functions(Elev8_Context *ctx)
{
#define REGISTER_FUNC(name) \
        Context::GetCurrent()->Global()->Set(String::NewSymbol(#name), FunctionTemplate::New(_ ## name, ctx->v8_self)->GetFunction());

    REGISTER_FUNC(require);
    REGISTER_FUNC(__require__);
    REGISTER_FUNC(modules);
    REGISTER_FUNC(print);

#undef REGISTER_FUNC
}

static void
_load_internal_modules(Elev8_Context *ctx)
{
    timer::RegisterModule(ctx->global);
    storage::RegisterModule(ctx->global);
    environment::RegisterModule(ctx->global);
    utils::RegisterModule(ctx->global);
    gadcon::RegisterModule(ctx->global);
}

void
elev8_init()
{
    eina_init();
    ecore_con_init();

    log_domain = eina_log_domain_register("elev8", EINA_COLOR_ORANGE);
    if (!log_domain)
      {
          fprintf(stderr, "Could not register elev8 log domain, using global\n");
          log_domain = EINA_LOG_DOMAIN_GLOBAL;
      }

    V8::AddMessageListener(_message_handler);
    V8::SetCaptureStackTraceForUncaughtExceptions(true, 10, StackTrace::kDetailed);

    siguser_handler = ecore_event_handler_add(ECORE_EVENT_SIGNAL_USER, _flush_garbage_collector, NULL);
}

void
elev8_shutdown()
{
    ecore_event_handler_del(siguser_handler);
    ecore_con_shutdown();
    eina_shutdown();
}

Elev8_Context *
elev8_context_add()
{
    HandleScope scope;
    Elev8_Context *ctx = (Elev8_Context *)calloc(1, sizeof(*ctx));

    if (!ctx) return NULL;

    ctx->global = Persistent<ObjectTemplate>::New(ObjectTemplate::New());

    ctx->context = Context::New(NULL, ctx->global);
    ctx->context->Enter();

    ctx->module_cache = Persistent<Object>::New(Object::New());
    ctx->v8_self = Persistent<Value>::New(External::New(ctx));

    _register_basic_functions(ctx);
    _load_internal_modules(ctx);

    ctx->context->Exit();

    return ctx;
}

void
elev8_context_del(Elev8_Context *ctx)
{
    if (!ctx) return;

    ctx->v8_self.Dispose();
    ctx->context.Dispose();
    ctx->module_cache.Dispose();
    ctx->global.Dispose();
    free(ctx);
}

void
elev8_context_leave(Elev8_Context *ctx)
{
    ctx->context->Exit();
}

void
elev8_context_enter(Elev8_Context *ctx)
{
    ctx->context->Enter();
}

Eina_Bool
elev8_context_script_exec(Elev8_Context *ctx, const char *path)
{
    Context::Scope s(ctx->context);
    HandleScope scope;
    TryCatch try_catch;

    Handle<String> source = _load_script_from_file(path);
    if (source.IsEmpty())
      {
         CRIT("Could not open script \"%s\"", path);
         return EINA_FALSE;
      }

    Local<Script> script = Script::New(source, String::New(path));
    if (try_catch.HasCaught())
      {
         CRIT("Could not compile script \"%s\": %s\n",
              path, *String::Utf8Value(try_catch.Exception()));
         return EINA_FALSE;
      }

    script->Run();
    if (try_catch.HasCaught())
      {
         CRIT("Could not run script \"%s\": %s\n",
              path, *String::Utf8Value(try_catch.Exception()));
         return EINA_FALSE;
      }

    return EINA_TRUE;
}
