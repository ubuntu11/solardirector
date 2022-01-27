
#ifndef edit_EXPORT_H
#define edit_EXPORT_H

#ifdef edit_BUILT_AS_STATIC
#  define edit_EXPORT
#  define EDIT_NO_EXPORT
#else
#  ifndef edit_EXPORT
#    ifdef edit_EXPORTS
        /* We are building this library */
#      define edit_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define edit_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef EDIT_NO_EXPORT
#    define EDIT_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef EDIT_DEPRECATED
#  define EDIT_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef EDIT_DEPRECATED_EXPORT
#  define EDIT_DEPRECATED_EXPORT edit_EXPORT EDIT_DEPRECATED
#endif

#ifndef EDIT_DEPRECATED_NO_EXPORT
#  define EDIT_DEPRECATED_NO_EXPORT EDIT_NO_EXPORT EDIT_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef EDIT_NO_DEPRECATED
#    define EDIT_NO_DEPRECATED
#  endif
#endif

#endif /* edit_EXPORT_H */
