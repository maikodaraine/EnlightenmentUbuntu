/*
 * elev8 - javascript for EFL
 *
 * The script's job is to prepare for the main loop to run
 * then exit
 */

#include <libgen.h>
#include <dlfcn.h>
#include <Ecore.h>
#include <elev8_common.h>
#include <unistd.h>
#include <v8-debug.h>
#include "args.h"
#include "environment.h"
#include "storage.h"
#include "timer.h"
#include "utils.h"
#include "utils_js.h"
#include "server.h"
#include "main.h"

using namespace v8;
int elev8_log_domain = -1;

#define MODLOAD_ENV "ELEV8_MODPATH"
#define MODLOAD_ENV_DEFAULT_DIRS ".:" PACKAGE_LIB_DIR

static Persistent<Object> module_cache;
static Persistent<ObjectTemplate> global;
static Persistent<Context> context;

enum ContextUseRule{
   CREATE_NEW_CONTEXT,
   USE_CURRENT_CONTEXT
};

static Handle<Value>
string_to_object(Handle<String> str)
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

static Handle<String>
object_to_string(Handle<Object> obj)
{
   HandleScope scope;

   Handle<Value> json = Context::GetCurrent()->Global()->Get(String::NewSymbol("JSON"));
   if (json.IsEmpty())
     return scope.Close(String::New(""));

   Handle<Value> stringify = json->ToObject()->Get(String::NewSymbol("stringify"));
   if (stringify.IsEmpty())
     return scope.Close(String::New(""));

   Handle<Function> func = Handle<Function>::Cast(stringify);
   Handle<Value> args[3] = { obj, Null(), String::New("    ") };

   Handle<String> dump = func->Call(Context::GetCurrent()->Global(), 3, args)->ToString();

   Local<String> constructor_name = obj->GetConstructorName();
   if (constructor_name != String::NewSymbol("Object"))
     return scope.Close(String::Concat(constructor_name, dump));

   return scope.Close(dump);
}

static Handle<Value>
print(const Arguments& args)
{
   HandleScope handle_scope;
   int argument_count;

   argument_count = args.Length();
   if (!argument_count)
     goto end;

   for (int i = 0; i < argument_count; i++)
     {
        if (args[i]->IsObject())
          fputs(*String::Utf8Value(object_to_string(args[i]->ToObject())), stdout);
        else
          fputs(*String::Utf8Value(args[i]), stdout);

        if (i < argument_count - 1)
          putchar(' ');
     }

end:
   putchar('\n');
   fflush(stdout);

   return handle_scope.Close(Undefined());
}

static bool
run_script(const char *filename)
{
   HandleScope handle_scope;

   Handle<String> source = string_from_file(filename);
   if (!source.IsEmpty())
     {
        compile_and_run(source, filename);
        return true;
     }

   ERR("Failed to read source %s", filename);
   return false;
}

static char *
find_module_file_name(char *module_name, const char *prefix, const char *type)
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
find_native_module_file_name(char *module_name)
{
   return find_module_file_name(module_name, "lib", "so");
}

inline static char *
find_js_module_file_name(char *module_name)
{
   char *tmp;
   char *path = find_module_file_name(module_name, "", "js");

   if (path)
     return path;

   if (asprintf(&tmp, "%s/", module_name) < 0)
     goto end;

   path = find_module_file_name((char *)"package", tmp, "json");
   free(tmp);

   if (path)
     {
        HandleScope scope;
        Handle<String> package_json = string_from_file(path);
        Handle<Value> package_obj = string_to_object(package_json);

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
module_native_load(char *module_name, Handle<Object> name_space, ContextUseRule)
{
   char *file_name = find_native_module_file_name(module_name);

   if (!file_name) return false;

   DBG("Loading native module: %s", file_name);

   // FIXME: Use Eina_Module here.
   void *handle = dlopen(file_name, RTLD_LAZY);
   if (!handle)
     {
        ERR("Could not dlopen(%s): %s", file_name, dlerror());
        free(file_name);
        return false;
     }

   void (*init_func)(Handle<Object> name_space);
   init_func = (void (*)(Handle<Object>))dlsym(handle, "RegisterModule");
   if (!init_func)
     {
        ERR("Could not dlsym(%p, RegisterModule): %s", handle, dlerror());
        free(file_name);
        dlclose(handle);
        return false;
     }

   DBG("Initializing native module: %s\n", file_name);
   init_func(name_space);

   name_space->Set(String::NewSymbol("__dl_handle"), External::Wrap(handle));
   name_space->Set(String::NewSymbol("__file_name"), String::New(file_name));

   free(file_name);
   return true;
}

static bool
module_js_load(char *module_name, Handle<Object> name_space, ContextUseRule context_use_rule)
{
   char *file_name = find_js_module_file_name(module_name);
   bool return_value = false;

   if (!file_name) return false;

   DBG("Loading JavaScript module: %s [%s context]", file_name, context_use_rule ? "current" : "new");

   Handle<String> mod_source = string_from_file(file_name);
   if (mod_source.IsEmpty())
     {
        free(file_name);
        return false;
     }

   HandleScope handle_scope;

   Persistent<Context> mod_context;
   if (context_use_rule == CREATE_NEW_CONTEXT)
     {
        global->Set(String::NewSymbol("exports"), name_space);
        mod_context = Context::New(NULL, global);
        mod_context->Enter();

        run_script(PACKAGE_LIB_DIR "/../init.js");
     }

   TryCatch try_catch;
   Local<Script> mod_script = Script::Compile(mod_source->ToString(), String::New(file_name));
   if (try_catch.HasCaught())
     {
        boom(try_catch);
        goto end;
     }

   mod_script->Run();
   if (try_catch.HasCaught())
     {
        boom(try_catch);
        goto end;
     }

   name_space->Set(String::NewSymbol("__file_name"), String::New(file_name));
   // FIXME: How to wrap mod_context so that t can be Disposed() properly?

   return_value = true;
end:
   free(file_name);

   if (context_use_rule == CREATE_NEW_CONTEXT)
     mod_context->Exit();

   return return_value;
}

static bool
load_module_with_type_hints(Handle<String> module_name, Local<Object> name_space, ContextUseRule context_use_rule)
{
   String::Utf8Value module_name_utf(module_name);

   if (module_name->Length() <= 3)
     {
        DBG("no extension found in module [%s] name, trying both types", *String::Utf8Value(module_name));
        goto end;
     }

   if (eina_str_has_suffix(*module_name_utf, ".js"))
     {
        *(*module_name_utf + module_name->Length() - 3) = '\0';
        DBG("module [%s] is javascript, running module_js_load(%s)", *String::Utf8Value(module_name), *module_name_utf);
        return module_js_load(*module_name_utf, name_space, context_use_rule);
     }

   if (eina_str_has_suffix(*module_name_utf, ".so"))
     {
        *(*module_name_utf + module_name->Length() - 3) = '\0';
        DBG("module [%s] is native, running module_native_load(%s)", *String::Utf8Value(module_name), *module_name_utf);
        return module_native_load(*module_name_utf, name_space, context_use_rule);
     }

   DBG("no extension provided for module [%s] trying to load native then js", *String::Utf8Value(module_name));
end:
   return module_native_load(*module_name_utf, name_space, context_use_rule)
       || module_js_load(*module_name_utf, name_space, context_use_rule);
}

static Handle<Value>
load_module(Handle<String> module_name, ContextUseRule context_use_rule)
{
   HandleScope scope;

   if (module_cache->HasOwnProperty(module_name))
     return scope.Close(module_cache->Get(module_name));

   Local<Object> name_space = (context_use_rule == CREATE_NEW_CONTEXT) ?
                Object::New() : Context::GetCurrent()->Global();

   if (load_module_with_type_hints(module_name, name_space, context_use_rule))
     {
        module_cache->Set(module_name, Persistent<Object>::New(name_space));
        return scope.Close(name_space);
     }

   Local<String> msg = String::Concat(String::New("Cannot load module: "), module_name);
   return scope.Close(ThrowException(Exception::Error(msg)));
}

static inline Handle<Value>
internal_require(const Arguments& args, ContextUseRule cur)
{
   HandleScope scope;
   if (args.Length() < 1)
     return scope.Close(ThrowException(Exception::Error(String::New("Module name missing"))));
   return scope.Close(load_module(args[0]->ToString(), cur));
}

static Handle<Value>
require(const Arguments& args)
{
   return internal_require(args, CREATE_NEW_CONTEXT);
}

static Handle<Value>
__require__(const Arguments& args)
{
   return internal_require(args, USE_CURRENT_CONTEXT);
}

static Handle<Value>
modules(const Arguments&)
{
   return module_cache;
}

static void
message(Handle<Message> message, Handle<Value>)
{
   boom(message, *String::Utf8Value(message->Get()));
}

static Eina_Bool
debug_message_handler_idler_cb(void *)
{
   DBG("Processing debug messages");
   Debug::ProcessDebugMessages();
   return ECORE_CALLBACK_CANCEL;
}

static void
debug_message_handler()
{
   ecore_idler_add(debug_message_handler_idler_cb, NULL);
}

static Eina_Bool
flush_garbage_collector(void *, int , void *)
{
   V8::LowMemoryNotification();
   return ECORE_CALLBACK_DONE;
}

void
load_elev8_modules()
{
   INF("Loading Elev8 modules");
   
   global = Persistent<ObjectTemplate>::New(ObjectTemplate::New());
   global->Set(String::NewSymbol("require"), FunctionTemplate::New(require));
   global->Set(String::NewSymbol("__require__"), FunctionTemplate::New(__require__));
   global->Set(String::NewSymbol("modules"), FunctionTemplate::New(modules));
   global->Set(String::NewSymbol("print"), FunctionTemplate::New(print));
   storage::RegisterModule(global);
   timer::RegisterModule(global);
   environment::RegisterModule(global);
   utils::RegisterModule(global);

   context = Context::New(NULL, global);
   context->Enter();
   module_cache = Persistent<Object>::New(Object::New());

   load_module(String::NewSymbol("elm"), CREATE_NEW_CONTEXT);
   load_module(String::NewSymbol("http"), CREATE_NEW_CONTEXT);

   INF("Elev8 modules loaded");
}

void
execute_elev8_script(char *script, int argc, char *argv[])
{
   V8::SetFlagsFromCommandLine(&argc, argv, true);

   if ((!run_script(PACKAGE_LIB_DIR "/../init.js")) ||
      (!run_script(script)))
     ecore_main_loop_quit();
}

int
main(int argc, char *argv[])
{
   int script_arg = 1;
   elev8_args args = { };

   eina_init();
   ecore_con_init();

   elev8_log_domain = eina_log_domain_register("elev8", EINA_COLOR_ORANGE);
   if (!elev8_log_domain)
     {
        printf("cannot set elev8 log domain\n");
        ERR( "could not register elev8 log domain.");
        elev8_log_domain = EINA_LOG_DOMAIN_GLOBAL;
     }
   INF("elev8 Logging initialized. %d", elev8_log_domain);

   V8::AddMessageListener(message);
   V8::SetCaptureStackTraceForUncaughtExceptions(true, 10, StackTrace::kDetailed);

   elev8_parse_argv(&args, argc, argv);

   if (argc < 2)
     {
        ERR("%s: Error: no input file specified.", argv[0]);
        printf("Usage:\n"
            "\t%s input_file.js\n"
            "\te.g. %s ../../data/javascript/button.js\n",
             argv[0], argv[0]);
        exit(-1);
     }
   else if (args.debug)
     {
        printf("Awaiting connection from debugger on port 5858.");
        fflush(stdout);
        Debug::EnableAgent("elev8", 5858, true);
        Debug::SetDebugMessageDispatchHandler(debug_message_handler);
        script_arg = 2;
     }
   else if (args.quit)
     exit(0);

   HandleScope handle_scope;

   if (args.server)
     server_start();
   else if (args.shutdown)
     server_shutdown();
   else if (args.connect)
     {
        script_arg = 2;
        server_spawn(script_arg, argc, argv);
     }
   else
     {
       load_elev8_modules();
       execute_elev8_script(argv[script_arg], argc, argv);
     }

   ecore_event_handler_add(ECORE_EVENT_SIGNAL_USER, flush_garbage_collector, NULL);
   ecore_main_loop_begin();

   if (Context::InContext()) context->Exit();
   context.Dispose();
   module_cache.Dispose();
   global.Dispose();

   ecore_con_shutdown();
   eina_shutdown();

   return 0;
}
