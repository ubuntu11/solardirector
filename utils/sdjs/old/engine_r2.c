#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jsapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <utime.h>

/* Function Prototypes */
JSBool js_engine_execute_file(JSContext *ctx, const char *file);
static void js_error_handler(JSContext *ctx, const char *msg, JSErrorReport *er);
static char *staterrorstr(int e);
static jsval printf_exception(JSContext *cx, char *msg, ...);
static size_t JSString_to_CString(JSString *str, char **msg);

/* Console object functions */
JSBool Console_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_writeln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_writeError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_writeErrorln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_readInt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_readFloat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool Console_readChar(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool File_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool File_static_create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool File_static_touch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool File_static_exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool File_exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool File_create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
JSBool File_touch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSClass js_global_object_class = {
  "System",
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

JSClass js_File_class = {
    "File",
    JSCLASS_HAS_PRIVATE,
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


JSFunctionSpec Console_methods[] = {
  {"write", Console_write, 0, 0, 0},
  {"writeln", Console_writeln, 0, 0, 0},
  {"writeError", Console_writeError, 0, 0, 0},
  {"writeErrorln", Console_writeErrorln, 0, 0, 0},
  {"read", Console_read, 0, 0, 0},
  {"readInt", Console_readInt, 0, 0, 0},
  {"readFloat", Console_readFloat, 0, 0, 0},
  {"readChar", Console_readChar, 0, 0, 0},
  {NULL},
};

struct _FileInformation {
    char *filename;
    FILE *fp;
};

typedef struct _FileInformation FileInformation;

JSFunctionSpec File_methods[] = {
    {"create", File_create, 0, 0 ,0},
    {"touch", File_touch, 0, 0, 0},
    {"exists", File_exists, 0, 0 ,0},
    {NULL}
};

JSFunctionSpec File_static_methods[] = {
    {"create", File_static_create, 0, 0 ,0},
    {"touch", File_static_touch, 0, 0, 0},
    {"exists", File_static_exists, 0, 0 ,0},
    {NULL}
};

int main(int argc, char **argv){
    char *fileToRun=NULL;
    JSRuntime *rt=NULL;
    JSContext *cx=NULL;
    JSObject *globalObj=NULL;
    JSObject *obj=NULL;

    if (argc > 1){
      fileToRun = argv[1];
    }
    
    rt = JS_NewRuntime(8 * 1024 * 1024);
    if (!rt){
      printf("Failed to initialize JS Runtime.\n");
      return 1;
    }

    cx = JS_NewContext(rt, 8L*1024L*1024L);
    if (!cx){
      printf("Failed to initialize JS Context.\n");
      JS_DestroyRuntime(rt);
      return 1;
    }
    
    JS_SetErrorReporter(cx, js_error_handler);
    
    globalObj = JS_NewObject(cx, &js_global_object_class, NULL, NULL);
    if (!globalObj){
      printf("Failed to create global object.\n");
      JS_DestroyContext(cx);
      JS_DestroyRuntime(rt);
      return 1;
    }

    JS_InitStandardClasses(cx, globalObj);
    
    /* Define our new object. */
    obj = JS_InitClass(cx, globalObj, NULL, &js_Console_class, NULL, 0, NULL, Console_methods, NULL, NULL);
    if (!obj){
      printf("Failed to create Console object.\n");
      JS_DestroyContext(cx);
      JS_DestroyRuntime(rt);
      return 1;
    }

    /* Define the file object. */
    obj = JS_InitClass(cx, globalObj, NULL, &js_File_class, File_ctor, 1, NULL, File_methods, NULL, File_static_methods);
    if (!obj){
        printf("Failed to create File object.\n");
        JS_DestroyContext(cx);
        JS_DestroyRuntime(rt);
        return 1;
    }


    if (fileToRun){
      if (js_engine_execute_file(cx, fileToRun)){
        printf("File %s has been successfully executed.\n", fileToRun);
      }
      else {
        printf("Failed to executed %s.\n", fileToRun);
      }
    }
    else {
      printf("Usage: %s file\n", argv[0]);
    }


    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    return 0;
}


JSBool js_engine_execute_file(JSContext *ctx, const char *file){
    JSScript *script;
    jsval returnValue;
    JSBool returnVal;
    JSObject *global = JS_GetGlobalObject(ctx);
    struct stat statinfo;

    if (file == NULL){
        return JS_FALSE;
    }

    if (stat(file, &statinfo) == -1){
        return JS_FALSE;
    }

    if (!S_ISREG(statinfo.st_mode)){
        return JS_FALSE;
    }

    script = JS_CompileFile(ctx, global, file);

    if (script == NULL){
        return JS_FALSE;
    }

    returnVal = JS_ExecuteScript(ctx, global, script, &returnValue);
    JS_DestroyScript(ctx, script);
    return returnVal;
}



static void js_error_handler(JSContext *ctx, const char *msg, JSErrorReport *er){
    char *pointer=NULL;
    char *line=NULL;
    int len;

    if (er->linebuf != NULL){
        len = er->tokenptr - er->linebuf + 1;
        pointer = malloc(len);
        memset(pointer, '-', len);
        pointer[len-1]='\0';
        pointer[len-2]='^';

        len = strlen(er->linebuf);
        line = malloc(len);
        strncpy(line, er->linebuf, len);
    }
    else {
        len=0;
        pointer = malloc(1);
        line = malloc(1);
        pointer[0]='\0';
        line[0] = '\0';
    }

    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')){ line[len-1]='\0'; len--; }

    printf("JS Error: %s\nFile: %s:%u\n", msg, er->filename, er->lineno);
    printf("%s\n%s\n", line, pointer);

    free(pointer);
    free(line);
}

/*****************************************/
/****** Begin Console object code. *******/
/*****************************************/

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

JSBool Console_writeln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  /* Just call the write function and then write out a \n */
  Console_write(cx, obj, argc, argv, rval);
  fwrite("\n", sizeof("\n"), 1, stdout);
  *rval = INT_TO_JSVAL(JSVAL_TO_INT(*rval)+1);
  return JS_TRUE;
}


JSBool Console_writeError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  if (argc < 1){
    *rval = INT_TO_JSVAL(0);
    return JS_TRUE;
  }
  else {
    int i;
    size_t amountWritten=0;
    for (i=0; i<argc; i++){
      JSString *val = JS_ValueToString(cx, argv[i]);
      char *str = JS_GetStringBytes(val); 
          size_t length = JS_GetStringLength(val); 
          amountWritten = fwrite(str, sizeof(*str), length, stderr); 
    }
    *rval = INT_TO_JSVAL(amountWritten);
    return JS_TRUE;
  }
}

JSBool Console_writeErrorln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  /* Just call the writeError function and then write out a \n */
  Console_writeError(cx, obj, argc, argv, rval);
  fwrite("\n", sizeof("\n"), 1, stderr);
  *rval = INT_TO_JSVAL(JSVAL_TO_INT(*rval)+1);
  return JS_TRUE;
}

JSBool Console_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  if (argc < 1){
    /* Read a full line off the console.  */
    char *finalBuff = NULL;
    char *newBuff = NULL;
    char tmpBuff[100];
    size_t totalLen = 0;
    int len = 0;
    do {
      memset(tmpBuff, 0, sizeof(tmpBuff));
      fgets(tmpBuff, sizeof(tmpBuff), stdin);
      len = strlen(tmpBuff);
      

      if (len > 0){
        char lastChar = tmpBuff[len-1];
        
        newBuff = JS_realloc(cx, finalBuff, totalLen+len+1);      
        if (newBuff == NULL){
          JS_free(cx, finalBuff);
          *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
          return JS_TRUE;
        }
        else {
          finalBuff = newBuff;
          memset(finalBuff+totalLen, 0, len);
        }
        
        strncat(finalBuff+totalLen, tmpBuff, len);
        totalLen += len;
        if (lastChar == '\n' || lastChar == '\r' || feof(stdin)){
          JSString *str = JS_NewString(cx, finalBuff, totalLen);
          *rval = STRING_TO_JSVAL(str);
          return JS_TRUE;
        }
      }
      else if (feof(stdin) && totalLen == 0){
        *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
        return JS_TRUE;
      }
      else if (totalLen > 0){
        JSString *str = JS_NewString(cx, finalBuff, totalLen);
        *rval = STRING_TO_JSVAL(str);
        return JS_TRUE;
      }
    } while (1);
  }
  else {
    int32 maxlen=0;
    if (JS_ValueToInt32(cx, argv[0], &maxlen) == JS_TRUE){
      JSString *ret=NULL;
      size_t amountRead = 0;
      char *newPointer = NULL;
      char *cstring = JS_malloc(cx, sizeof(*cstring)*(maxlen+1));
      if (cstring == NULL){
        *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
        return JS_TRUE;
      }
      memset(cstring, 0, sizeof(*cstring)*(maxlen+1));
      amountRead = fread(cstring, sizeof(*cstring), maxlen, stdin);
      newPointer = JS_realloc(cx, cstring, amountRead);
      if (newPointer == NULL){
        JS_free(cx, cstring);
        *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
        return JS_TRUE;
      }
      else {
        cstring = newPointer;
      }
      
      ret = JS_NewString(cx, cstring, sizeof(*cstring)*amountRead);
      *rval = STRING_TO_JSVAL(ret);
      return JS_TRUE;
    }
    else {
      *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
      return JS_TRUE;
    }
  }
}

JSBool Console_readInt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  int32 readinteger = 0;
  if (fscanf(stdin, "%d", &readinteger) == 1){
    if (JS_NewNumberValue(cx, readinteger, rval) == JS_TRUE){
      return JS_TRUE;
    }
  }
  *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
  return JS_TRUE;
}

JSBool Console_readFloat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  jsdouble readfloat = 0;
  if (fscanf(stdin, "%lf", &readfloat) == 1){
    if (JS_NewDoubleValue(cx, readfloat, rval) == JS_TRUE){
      return JS_TRUE;
    }
  }
 *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
  return JS_TRUE;
}


JSBool Console_readChar(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
  JSString *str=NULL;
  char ch, *ptr=NULL;
  if (feof(stdin)){
    *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
    return JS_TRUE;
  }
  
  ch=fgetc(stdin);
  if (ch == EOF){
    *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
    return JS_TRUE;
  }
  
  ptr = JS_malloc(cx, sizeof(char)*1);
  if (ptr == NULL){
    *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
    return JS_TRUE;
  }
  
  *ptr = ch;
  str = JS_NewString(cx, ptr, sizeof(char)*1);
  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}


JSBool File_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    if (argc < 1){
        char *str = strdup("Not enough parameters to File constructor.");
        JSString *exceptionDescription = JS_NewString(cx, str, strlen(str));
        JS_SetPendingException(cx, STRING_TO_JSVAL(exceptionDescription));
        return JS_FALSE;
    }
    else {
		char *file=NULL;
		size_t fileNameLength=0;

        FileInformation *info = malloc(sizeof(*info));
        info->fp = NULL;
        info->filename = NULL;
			
		fileNameLength = JSString_to_CString(JS_ValueToString(cx, argv[0]), &file);

        info->filename = strdup(file);
        JS_SetPrivate(cx, obj, info);
    }
    return JS_TRUE;
}

JSBool File_static_create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    if (argc < 1){
        jsval exception = printf_exception(cx, "Not enough parameters to create file.  File.create(<filename>)");
        JS_SetPendingException(cx, exception);
        return JS_FALSE;
    }
    else {
		char *str=NULL;
		size_t length=0;
        struct stat existsCheck;

		length = JSString_to_CString(JS_ValueToString(cx, argv[0]), &str);


        /* First see if the file already exists.  If it does, throw an exception. */

        if (stat(str, &existsCheck) == 0){
			jsval exception = printf_exception(cx, "File '%s' already exists, cannot create.", str);
            JS_SetPendingException(cx, exception);
            return JS_FALSE;
        }
        else if (errno == ENOENT){
            FILE *fp = NULL;
            fp = fopen(str, "w");
            if (fp){
                fclose(fp);            
                *rval = BOOLEAN_TO_JSVAL(JS_TRUE);
            }
            else {
                *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
            }
            return JS_TRUE;
        }
        else {
			char *msg = staterrorstr(errno);
            jsval exception = printf_exception(cx, "%s", msg);
			free(msg);
            JS_SetPendingException(cx, STRING_TO_JSVAL(exception));
            return JS_FALSE;
        }
    }
}

static char *staterrorstr(int e){
    char *msg = NULL;
    switch (e){
        case EACCES:
            msg = strdup("Permission denied to check file.");
            break;
        case EBADF:
            msg = strdup("Bad file descriptor");
            break;
        case ELOOP:
            msg = strdup("Symbolic link loop encountered.");
            break;
        case ENAMETOOLONG:
            msg = strdup("File name too long, not supported.");
            break;
        case ENOMEM:
            msg = strdup("Out of memory.");
            break;
        case ENOTDIR:
            msg = strdup("Path component not a directory.");
            break;
        default:
            msg = strdup("Unknown error occured.");
    }
    return msg;
}

JSBool File_static_touch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	if (argc < 1){
		jsval exception = printf_exception(cx, "Incorrect number of agruments passed.  Expected one.");
		JS_SetPendingException(cx, exception);
	}
	else {
		struct stat existsCheck;
		char *file = NULL;
		JSString_to_CString(JS_ValueToString(cx, argv[0]), &file);

		if (stat(file, &existsCheck) == 0){
			struct utimbuf times;
			times.actime = time(NULL);
			times.modtime = time(NULL);

			if (utime(file, &times) == 0){
				*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			}
			else {
				char *msg = staterrorstr(errno);
				jsval exception = printf_exception(cx, "touch failed.  (%d) %s", errno, msg);
				free(msg);
				JS_SetPendingException(cx, exception);
				return JS_FALSE;
			}				
		}
		else if (errno == ENOENT){
			FILE *fp = fopen(file, "w");
			if (fp){
				fclose(fp);
				*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
			}
			else {
				char *msg = staterrorstr(errno);
				jsval exception = printf_exception(cx, "touch failed, could not create file.  (%d) %s", errno, msg);
				free(msg);
				JS_SetPendingException(cx, exception);
				return JS_FALSE;
			}
		}
		else {
			char *msg = staterrorstr(errno);
			jsval exception = printf_exception(cx, "Existance check failed (%d) %s", errno, msg);
			free(msg);
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}
	}
    return JS_TRUE;
}

JSBool File_static_exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	if (argc < 1){
		jsval exception = printf_exception(cx, "Incorrect number of agruments passed.  Expected one.");
		JS_SetPendingException(cx, exception);
	}
	else {
		struct stat existsCheck;
		char *file = NULL;
		JSString_to_CString(JS_ValueToString(cx, argv[0]), &file);

		if (stat(file, &existsCheck) == 0){
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		}
		else if (errno == ENOENT){
			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		}
		else {
			char *msg = staterrorstr(errno);
			jsval exception = printf_exception(cx, "Existance check failed (%d) %s", errno, msg);
			free(msg);
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}
	}
    return JS_TRUE;
}

JSBool File_create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
    /* First see if the file already exists.  If it does, throw an exception. */
    struct stat existsCheck;
    FileInformation *info = (FileInformation*)JS_GetPrivate(cx, obj);
    
    if (stat(info->filename, &existsCheck) == 0){
		jsval exception = printf_exception(cx, "Cannot create file '%s'.  A file by that name already exists.", info->filename);
        JS_SetPendingException(cx, exception);
        return JS_FALSE;
    }
    else if (errno == ENOENT){
        FILE *fp = NULL;
        fp = fopen(info->filename, "w");
        if (fp){
            fclose(fp);            
            *rval = BOOLEAN_TO_JSVAL(JS_TRUE);
        }
        else {
            *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
        }
        return JS_TRUE;
    }
    else {
        char *msg = staterrorstr(errno);
		jsval exception = printf_exception(cx, "Creation failed.  (%d) %s", errno, msg);
		free(msg);
		JS_SetPendingException(cx, exception);
        return JS_FALSE;
    }
	return JS_TRUE;
}

JSBool File_touch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	struct stat existsCheck;
    FileInformation *info = (FileInformation*)JS_GetPrivate(cx, obj);
    
	if (stat(info->filename, &existsCheck) == 0){
		struct utimbuf times;
		times.actime = time(NULL);
		times.modtime = time(NULL);

		if (utime(info->filename, &times) == 0){
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		}
		else {
			char *msg = staterrorstr(errno);
			jsval exception = printf_exception(cx, "touch failed.  (%d) %s", errno, msg);
			free(msg);
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}				
	}
	else if (errno == ENOENT){
		FILE *fp = fopen(info->filename, "w");
		if (fp){
			fclose(fp);
			*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		}
		else {
			char *msg = staterrorstr(errno);
			jsval exception = printf_exception(cx, "touch failed, could not create file.  (%d) %s", errno, msg);
			free(msg);
			JS_SetPendingException(cx, exception);
			return JS_FALSE;
		}
	}
	else {
		char *msg = staterrorstr(errno);
		jsval exception = printf_exception(cx, "Existance check failed (%d) %s", errno, msg);
		free(msg);
		JS_SetPendingException(cx, exception);
		return JS_FALSE;
	}
    return JS_TRUE;
}

JSBool File_exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval){
	/* First see if the file already exists.  If it does, throw an exception. */
    struct stat existsCheck;
    FileInformation *info = (FileInformation*)JS_GetPrivate(cx, obj);
    
    if (stat(info->filename, &existsCheck) == 0){
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
    }
    else if (errno == ENOENT){
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
    }
    else {
        char *msg = staterrorstr(errno);
		jsval exception = printf_exception(cx, "Existance check failed. (%d) %s", errno, msg);
		free(msg);
		JS_SetPendingException(cx, exception);
        return JS_FALSE;
    }
	return JS_TRUE;
}


static size_t JSString_to_CString(JSString *str, char **msg){
	*msg = JS_GetStringBytes(str); /* Then convert it to a C-style string. */
	return JS_GetStringLength(str); /* Get the length of the string, # of chars. */
}

static jsval printf_exception(JSContext *cx, char *msg, ...){
	char *str=NULL;
	int len = 0;
	va_list args;
	JSString *exceptionDescription;

	va_start(args, msg);
	len = vsnprintf(NULL, 0, msg, args)+1;
	va_end(args);

	str = JS_malloc(cx, len);
	memset(str, 0, len);

	va_start(args, msg);
	vsnprintf(str, len, msg, args);
	va_end(args);

	exceptionDescription = JS_NewString(cx, str, len);
	return STRING_TO_JSVAL(exceptionDescription);
}
