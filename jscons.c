#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jsapi.h>
#include <sys/types.h>
#include <sys/stat.h>

JSClass js_Console_class = {
  "Console",
  0,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub,
  JS_FinalizeStub,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool Console_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  if (argc < 1){
    /* No arguments passed in, so do nothing. */
    /* We still want to return JS_TRUE though, other wise an exception will be thrown by the engine. */
    *rval = INT_TO_JSVAL(0); /* Send back a return value of 0. */
    return JS_TRUE;
  }
  else {
    /* "Hidden" feature.  The function will accept multiple arguments.  Each one is considered to be a string
       and will be written to the console accordingly.  This makes it possible to avoid concatenation in the code
       by using the Console.write function as so:
       Console.write("Welcome to my application", name, ". I hope you enjoy the ride!");
       */
    int i;
    size_t amountWritten=0;
    for (i=0; i<argc; i++){
      JSString *val = JS_ValueToString(cx, argv[i]); /* Convert the value to a javascript string. */
      char *str = JS_GetStringBytes(val); /* Then convert it to a C-style string. */
      size_t length = JS_GetStringLength(val); /* Get the length of the string, # of chars. */
      amountWritten = fwrite(str, sizeof(*str), length, stdout); /* write the string to stdout. */
    }
    *rval = INT_TO_JSVAL(amountWritten); /* Set the return value to be the number of bytes/chars written */
    return JS_TRUE;
  }
}

JSFunctionSpec Console_methods[] = {
  {"write", Console_write, 0, 0, 0},
  {NULL},
};

