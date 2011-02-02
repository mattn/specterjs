#include <webkit/webkit.h>
#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static WebKitWebView *wv;
static WebKitWebFrame *wf;
static JSObjectRef jsobj_specter;
static JSStringRef jsstr_state;
static JSObjectRef jsobj_args;
static JSObjectRef jsfunc_specter_open;
static JSObjectRef jsfunc_specter_exit;
static JSObjectRef jsfunc_specter_render;
static JSObjectRef jsfunc_specter_sleep;
static gchar* script;
static const char* propertyNames[] = {
  "state", "loadStatus", "args", "content", "userAgent", "viewportSize",
  "open", "exit", "render", "sleep",
  NULL };

static JSValueRef
specter_open(JSContextRef ctx, JSObjectRef functionObject,
        JSObjectRef thisObject, size_t argumentCount,
        const JSValueRef arguments[], JSValueRef* exception) {
  if (argumentCount > 0) {
    JSStringRef url = JSValueToStringCopy(ctx, arguments[0], NULL);
    size_t size = JSStringGetMaximumUTF8CStringSize(url);
    char str[size];
    JSStringGetUTF8CString(url, str, size);
    webkit_web_view_load_uri(wv, str);
    JSStringRelease(url);
  }
  return JSValueMakeUndefined(ctx);
}

static JSValueRef
specter_exit(JSContextRef ctx, JSObjectRef functionObject,
        JSObjectRef thisObject, size_t argumentCount,
        const JSValueRef arguments[], JSValueRef* exception) {
  int code = 0;
  if (argumentCount > 0) {
    code = (int) JSValueToNumber(ctx, arguments[0], NULL);
  }
  gtk_widget_destroy(GTK_WIDGET(wv));
  gtk_exit(code);
  return JSValueMakeUndefined(ctx);
}

static JSValueRef
specter_render(JSContextRef ctx, JSObjectRef functionObject,
        JSObjectRef thisObject, size_t argumentCount,
        const JSValueRef arguments[], JSValueRef* exception) {
  if (argumentCount > 0) {
    JSStringRef filename = JSValueToStringCopy(ctx, arguments[0], NULL);
    size_t size = JSStringGetMaximumUTF8CStringSize(filename);
    char str[size];
    JSStringGetUTF8CString(filename, str, size);
    GtkPrintOperation* op = gtk_print_operation_new();
    GError* error = NULL;
    gtk_print_operation_set_export_filename(op, str);
    webkit_web_frame_print_full(wf, op,
            GTK_PRINT_OPERATION_ACTION_EXPORT, &error);
    JSStringRelease(filename);
  }
  return JSValueMakeUndefined(ctx);
}

static double time_diff(struct timeval *st, struct timeval *et) {
  return (double) (et->tv_sec - st->tv_sec) * 1000000
      + (et->tv_usec - st->tv_usec);
}



static JSValueRef
specter_sleep(JSContextRef ctx, JSObjectRef functionObject,
        JSObjectRef thisObject, size_t argumentCount,
        const JSValueRef arguments[], JSValueRef* exception) {
  int ms = 0;
  if (argumentCount > 0) {
    ms = (int) JSValueToNumber(ctx, arguments[0], NULL);
  }
  struct timeval t1, t2;
  gettimeofday(&t1, NULL);
  while (TRUE) {
    gtk_main_iteration_do(FALSE);
    gettimeofday(&t2, NULL);
    if (time_diff(&t1, &t2) > ms * 1000)
      break;
  }
  return JSValueMakeUndefined(ctx);
}

static void
specter_getPropertyNames(JSContextRef ctx, JSObjectRef object,
        JSPropertyNameAccumulatorRef names) {
  JSStringRef name;
  const char** pn = propertyNames;
  while (*pn) {
    name = JSStringCreateWithUTF8CString(*pn);
    JSPropertyNameAccumulatorAddName(names, name);
    JSStringRelease(name);
    pn++;
  }
}

static bool
specter_hasProperty(JSContextRef ctx, JSObjectRef object, JSStringRef name) {
  const char* const *pn = propertyNames;
  while (*pn) {
    if (JSStringIsEqualToUTF8CString(name, *pn))
      return TRUE;
    pn++;
  }
  return FALSE;
}
static JSValueRef
specter_getProperty(JSContextRef ctx, JSObjectRef object,
        JSStringRef name, JSValueRef* exception) {
  if (JSStringIsEqualToUTF8CString(name, "loadStatus")) {
    JSStringRef ret;
    switch ((int) webkit_web_view_get_main_frame(wv)) {
      case WEBKIT_LOAD_PROVISIONAL:
        ret = JSStringCreateWithUTF8CString("provisional");
        break;
      case WEBKIT_LOAD_COMMITTED:
        ret = JSStringCreateWithUTF8CString("committed");
        break;
      case WEBKIT_LOAD_FINISHED:
        ret = JSStringCreateWithUTF8CString("finished");
        break;
      case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
        ret = JSStringCreateWithUTF8CString("first-visually-non-empty-layout");
        break;
      case WEBKIT_LOAD_FAILED:
        ret = JSStringCreateWithUTF8CString("failed");
        break;
      default:
        ret = JSStringCreateWithUTF8CString("unknown");
        break;
    }
    return JSValueMakeString(ctx, ret);
  } else if (JSStringIsEqualToUTF8CString(name, "args")) {
    return jsobj_args;
  } else if (JSStringIsEqualToUTF8CString(name, "state")) {
    return JSValueMakeString(ctx, jsstr_state);
  } else if (JSStringIsEqualToUTF8CString(name, "open")) {
    return jsfunc_specter_open;
  } else if (JSStringIsEqualToUTF8CString(name, "exit")) {
    return jsfunc_specter_exit;
  } else if (JSStringIsEqualToUTF8CString(name, "render")) {
    return jsfunc_specter_render;
  } else if (JSStringIsEqualToUTF8CString(name, "sleep")) {
    return jsfunc_specter_sleep;
  } else if (JSStringIsEqualToUTF8CString(name, "content")) {
    WebKitWebDataSource* source = webkit_web_frame_get_data_source(wf);
    GString* str = webkit_web_data_source_get_data(source);
    JSStringRef ret = JSStringCreateWithUTF8CString(str->str);
    return JSValueMakeString(ctx, ret);
  } else if (JSStringIsEqualToUTF8CString(name, "userAgent")) {
    WebKitWebSettings* settings = webkit_web_view_get_settings(wv);
    const gchar* ua = webkit_web_settings_get_user_agent(settings);
    JSStringRef ret = JSStringCreateWithUTF8CString(ua);
    return JSValueMakeString(ctx, ret);
  } else if (JSStringIsEqualToUTF8CString(name, "viewportSize")) {
    gint width, height;
    gtk_widget_get_size_request(GTK_WIDGET(wv), &width, &height);
    JSObjectRef ret = JSObjectMake(ctx, NULL, NULL);
    JSStringRef jstr;
    jstr = JSStringCreateWithUTF8CString("width");
    JSObjectSetProperty(ctx, ret, jstr, JSValueMakeNumber(ctx, width),
            kJSPropertyAttributeDontDelete, NULL);
    JSStringRelease(jstr);
    jstr = JSStringCreateWithUTF8CString("height");
    JSObjectSetProperty(ctx, ret, jstr, JSValueMakeNumber(ctx, height),
            kJSPropertyAttributeDontDelete, NULL);
    JSStringRelease(jstr);
    return ret;
  } else {
    JSStringRef message = JSStringCreateWithUTF8CString("Property not found.");
    JSValueRef exceptionString = JSValueMakeString(ctx, message);
    JSStringRelease(message);
    *exception = JSValueToObject(ctx, exceptionString, NULL);
    return NULL;
  }

  return JSValueMakeUndefined(ctx);
}

static bool
specter_setProperty(JSContextRef ctx, JSObjectRef object,
        JSStringRef name, JSValueRef value, JSValueRef *exception) {
  if (JSStringIsEqualToUTF8CString(name, "state")) {
    if (jsstr_state) JSStringRelease(jsstr_state);
    jsstr_state = JSValueToStringCopy(ctx, value, NULL);
    return TRUE;
  } else if (JSStringIsEqualToUTF8CString(name, "content")) {
    JSStringRef content = JSValueToStringCopy(ctx, value, NULL);
    size_t size = JSStringGetMaximumUTF8CStringSize(content);
    char str[size+1];
    str[size] = 0;
    JSStringGetUTF8CString(content, str, size);
    webkit_web_view_load_string(wv, str, "text/html", "utf-8", NULL);
    JSStringRelease(content);
    return TRUE;
  } else if (JSStringIsEqualToUTF8CString(name, "userAgent")) {
    JSStringRef userAgent = JSValueToStringCopy(ctx, value, NULL);
    size_t size = JSStringGetMaximumUTF8CStringSize(userAgent);
    char str[size+1];
    str[size] = 0;
    JSStringGetUTF8CString(userAgent, str, size);
    WebKitWebSettings* settings = webkit_web_view_get_settings(wv);
    g_object_set(G_OBJECT(settings), "user-agent", str, NULL);
    JSStringRelease(userAgent);
    return TRUE;
  } else if (JSStringIsEqualToUTF8CString(name, "viewportSize")) {
    JSObjectRef viewportSize = JSValueToObject(ctx, value, NULL);
    JSValueRef pv;
    JSStringRef jstr;
    jstr = JSStringCreateWithUTF8CString("width");
    pv = JSObjectGetProperty(ctx, viewportSize, jstr, NULL);
    JSStringRelease(jstr);
    gint width = JSValueToNumber(ctx, pv, NULL);
    jstr = JSStringCreateWithUTF8CString("height");
    pv = JSObjectGetProperty(ctx, viewportSize, jstr, NULL);
    JSStringRelease(jstr);
    gint height = JSValueToNumber(ctx, pv, NULL);
    gtk_widget_set_size_request(GTK_WIDGET(wv), width, height);
    return TRUE;
  }
  JSStringRef message = JSStringCreateWithUTF8CString("Property not found.");
  JSValueRef exceptionString = JSValueMakeString(ctx, message);
  JSStringRelease(message);
  *exception = JSValueToObject(ctx, exceptionString, NULL);
  return FALSE;
}

static void
window_object_cleared(WebKitWebView* wv, WebKitWebFrame* wf,
        JSGlobalContextRef ctx, JSObjectRef windowObject, gpointer data){
  JSStringRef jstr = JSStringCreateWithUTF8CString("specter");
  JSObjectSetProperty(ctx, windowObject, jstr, jsobj_specter,
          kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, NULL);
  JSStringRelease(jstr);
}

static void
load_finished(WebKitWebView* wv, WebKitWebFrame* wf, gpointer user_data) {
  webkit_web_view_execute_script(wv, script);
}

static gboolean
console_message(WebKitWebView* wv, gchar *message, gint line,
        gchar *source_id, gpointer user_data) {
  fprintf(stdout, "%s\n", message);
  return TRUE;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "specterjs script.js\n\n");
    return 1;
  }

  gtk_init(0, NULL);

  wv = (WebKitWebView*) webkit_web_view_new();
  wf = (WebKitWebFrame*) webkit_web_view_get_main_frame(wv);

  char* proxy = getenv("HTTP_PROXY");
  if (proxy) {
    SoupURI *uri = soup_uri_new(proxy);
    g_object_set(G_OBJECT(webkit_get_default_session()),
            SOUP_SESSION_PROXY_URI, uri, NULL);
  }

  webkit_web_frame_get_global_context(wf);
  JSGlobalContextRef ctx = webkit_web_frame_get_global_context(wf);
  JSObjectRef global = JSContextGetGlobalObject(ctx);

  JSClassDefinition classDefinition = kJSClassDefinitionEmpty;
  classDefinition.attributes = kJSClassAttributeNone ;
  classDefinition.className = "SpecterJS";
  classDefinition.hasProperty = specter_hasProperty;
  classDefinition.getProperty = specter_getProperty;
  classDefinition.setProperty = specter_setProperty;
  classDefinition.getPropertyNames = specter_getPropertyNames;
  JSClassRef classSubstance = JSClassCreate(&classDefinition);
  jsobj_specter = JSObjectMake(ctx, classSubstance, NULL);
  JSStringRef jstr = JSStringCreateWithUTF8CString("specter");
  JSObjectSetProperty(ctx, global, jstr, jsobj_specter,
          kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, NULL);
  JSStringRelease(jstr);
  JSValueProtect(ctx, jsobj_specter);

  jstr = JSStringCreateWithUTF8CString("open");
  jsfunc_specter_open = JSObjectMakeFunctionWithCallback(
          ctx, jstr, specter_open);
  JSStringRelease(jstr);
  JSValueProtect(ctx, jsfunc_specter_open);

  jstr = JSStringCreateWithUTF8CString("exit");
  jsfunc_specter_exit = JSObjectMakeFunctionWithCallback(
          ctx, jstr, specter_exit);
  JSStringRelease(jstr);
  JSValueProtect(ctx, jsfunc_specter_exit);

  jstr = JSStringCreateWithUTF8CString("render");
  jsfunc_specter_render = JSObjectMakeFunctionWithCallback(
          ctx, jstr, specter_render);
  JSStringRelease(jstr);
  JSValueProtect(ctx, jsfunc_specter_render);

  jstr = JSStringCreateWithUTF8CString("sleep");
  jsfunc_specter_sleep = JSObjectMakeFunctionWithCallback(
          ctx, jstr, specter_sleep);
  JSStringRelease(jstr);
  JSValueProtect(ctx, jsfunc_specter_sleep);

  JSValueRef argValue[argc];
  int n;
  for (n = 0; n < argc; n++) {
    jstr = JSStringCreateWithUTF8CString(argv[n]);
    argValue[n] = JSValueMakeString(ctx, jstr);
  }
  jsobj_args = JSObjectMakeArray(ctx, argc, argValue, NULL);
  JSValueProtect(ctx, jsobj_args);

  g_signal_connect(G_OBJECT(wv), "window-object-cleared",
          G_CALLBACK(window_object_cleared), NULL);
  g_signal_connect(G_OBJECT(wv), "load-finished",
          G_CALLBACK(load_finished), NULL);
  g_signal_connect(G_OBJECT(wv), "console-message",
          G_CALLBACK(console_message), NULL);

  WebKitWebSettings* settings = webkit_web_view_get_settings(wv);
    g_object_set(G_OBJECT(settings),
                 "enable-private-browsing", TRUE,
                 //"enable-developer-extras", FALSE,
                 //"enable-spell-checking", TRUE,
                 "enable-html5-database", TRUE,
                 "enable-html5-local-storage", TRUE,
                 //"enable-xss-auditor", FALSE,
                 //"enable-spatial-navigation", FALSE,
                 "javascript-can-access-clipboard", TRUE,
                 //"javascript-can-open-windows-automatically", TRUE,
                 //"enable-offline-web-application-cache", TRUE,
                 //"enable-universal-access-from-file-uris", TRUE,
                 "enable-scripts", TRUE,
                 //"enable-dom-paste", TRUE,
                 //"default-font-family", "Times",
                 //"monospace-font-family", "Courier",
                 //"serif-font-family", "Times",
                 //"sans-serif-font-family", "Helvetica",
                 //"default-font-size", 16,
                 //"default-monospace-font-size", 13,
                 //"minimum-font-size", 1,
                 //"enable-caret-browsing", FALSE,
                 //"enable-page-cache", FALSE,
                 "auto-resize-window", TRUE,
                 "enable-java-applet", FALSE,
                 "enable-plugins", TRUE,
                 //"editing-behavior", WEBKIT_EDITING_BEHAVIOR_WINDOWS,
                 NULL);

  GError* error = NULL;
  g_file_get_contents(argv[1], &script, NULL, &error);
  if (error) {
    fprintf(stderr, "%s\n", error->message);
  } else {
    webkit_web_view_execute_script(wv, script);
    gtk_main();
    g_free(script);
  }

  return 0;
}
/*
 TODO:
   args (array)
   content (string)
   loadStatus (string)
   state (string)
   sleep (number)
   exit (number)
   userAgent (string)
   - viewportSize (object)

   render
*/

// vim:set et ts=4
