#pragma once

#ifndef PP_CONCAT
    #define PP_CONCAT(x, y) x ## y
#endif

#ifndef PP_CONCAT_VAL
    #define PP_CONCAT_VAL(x, y) PP_CONCAT(x, y)
#endif

#ifndef PP_STR
    #define PP_STR(...) #__VA_ARGS__
#endif

#ifndef PP_STR_VAL
    #define PP_STR_VAL(...) PP_STR(__VA_ARGS__)
#endif

#ifndef PP_ARG_N
    #define PP_ARG_N(                                         \
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
        _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
        _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
        _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
        _61, _62, _63, _64, N, ...) N
#endif

#ifndef PP_NARG
    #define PP_NARG(...) \
        PP_ARG_N(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, \
        54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, \
        24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#ifndef PP_EVAL_0
    #define PP_EVAL_0(...) __VA_ARGS__
#endif
#ifndef PP_EVAL_1
    #define PP_EVAL_1(...) PP_EVAL_0(PP_EVAL_0(PP_EVAL_0(__VA_ARGS__)))
#endif
#ifndef PP_EVAL_2
    #define PP_EVAL_2(...) PP_EVAL_1(PP_EVAL_1(PP_EVAL_1(__VA_ARGS__)))
#endif
#ifndef PP_EVAL_3
    #define PP_EVAL_3(...) PP_EVAL_2(PP_EVAL_2(PP_EVAL_2(__VA_ARGS__)))
#endif
#ifndef PP_EVAL_4
    #define PP_EVAL_4(...) PP_EVAL_3(PP_EVAL_3(PP_EVAL_3(__VA_ARGS__)))
#endif
#ifndef PP_EVAL
    #define PP_EVAL(...)  PP_EVAL_4(PP_EVAL_4(PP_EVAL_4(__VA_ARGS__)))
#endif

#ifndef PP_FOREACH_END
    #define PP_FOREACH_END(...)
#endif
#ifndef PP_FOREACH_OUT
    #define PP_FOREACH_OUT
#endif
#ifndef PP_FOREACH_COMMA
    #define PP_FOREACH_COMMA ,
#endif

#ifndef PP_FOREACH_GET_END2
    #define PP_FOREACH_GET_END2() 0, PP_FOREACH_END
#endif
#ifndef PP_FOREACH_GET_END1
    #define PP_FOREACH_GET_END1(...) PP_FOREACH_GET_END2
#endif
#ifndef PP_FOREACH_GET_END
    #define PP_FOREACH_GET_END(...) PP_FOREACH_GET_END1
#endif
#ifndef PP_FOREACH_NEXT0
    #define PP_FOREACH_NEXT0(test, next, ...) next PP_FOREACH_OUT
#endif
#ifndef PP_FOREACH_NEXT1
    #define PP_FOREACH_NEXT1(test, next) PP_FOREACH_NEXT0(test, next, 0)
#endif
#ifndef PP_FOREACH_NEXT
    #define PP_FOREACH_NEXT(test, next)  PP_FOREACH_NEXT1(PP_FOREACH_GET_END test, next)
#endif

#ifndef PP_FOREACH_0
    #define PP_FOREACH_0(f, x, peek, ...) f(x) PP_FOREACH_NEXT(peek, PP_FOREACH_1)(f, peek, __VA_ARGS__)
#endif
#ifndef PP_FOREACH_1
    #define PP_FOREACH_1(f, x, peek, ...) f(x) PP_FOREACH_NEXT(peek, PP_FOREACH_0)(f, peek, __VA_ARGS__)
#endif

#ifndef PP_FOREACH_LIST_NEXT1
    #define PP_FOREACH_LIST_NEXT1(test, next) PP_FOREACH_NEXT0(test, PP_FOREACH_COMMA next, 0)
#endif
#ifndef PP_FOREACH_LIST_NEXT
    #define PP_FOREACH_LIST_NEXT(test, next)  PP_FOREACH_LIST_NEXT1(PP_FOREACH_GET_END test, next)
#endif

#ifndef PP_FOREACH_LIST0
    #define PP_FOREACH_LIST0(f, x, peek, ...) f(x) PP_FOREACH_LIST_NEXT(peek, PP_FOREACH_LIST1)(f, peek, __VA_ARGS__)
#endif
#ifndef PP_FOREACH_LIST1
    #define PP_FOREACH_LIST1(f, x, peek, ...) f(x) PP_FOREACH_LIST_NEXT(peek, PP_FOREACH_LIST0)(f, peek, __VA_ARGS__)
#endif

/*
 * Applies the function macro `f` to each of the remaining parameters.
 * Taken from https://github.com/swansontec/map-macro/blob/master/map.h
 */
#ifndef PP_FOREACH
    #define PP_FOREACH(f, ...) PP_EVAL(PP_FOREACH_1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#endif
