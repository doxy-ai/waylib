#ifndef MACROUTIL_H
#define MACROUTIL_H

// From: https://stackoverflow.com/a/47425231
//-----------------------------------------------------------------------------
// OVERLOADED_MACRO
//
// used to create other macros with overloaded argument lists
//
// Example Use:
// #define myMacro(...) OVERLOADED_MACRO( myMacro, __VA_ARGS__ )
// #define myMacro0() someFunc()
// #define myMacro1( arg1 ) someFunc( arg1 )
// #define myMacro2( arg1, arg2 ) someFunc( arg1, arg2 )
//
// myMacro();
// myMacro(1);
// myMacro(1,2);
//
// Note the numerical suffix on the macro names,
// which indicates the number of arguments.
// That is the REQUIRED naming convention for your macros.
//
//-----------------------------------------------------------------------------

// OVERLOADED_MACRO
// derived from: https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
// replaced use of _COUNT_ARGS macro with VA_NUM_ARGS defined below
// to support of zero argument overloads
#define OVERLOADED_MACRO(M, ...) _OVR(M, VA_NUM_ARGS(__VA_ARGS__)) (__VA_ARGS__)
#define _OVR(macroName, number_of_args)   _OVR_EXPAND(macroName, number_of_args)
#define _OVR_EXPAND(macroName, number_of_args)    macroName##number_of_args
//#define _COUNT_ARGS(...)  _ARG_PATTERN_MATCH(__VA_ARGS__, 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)
#define _ARG_PATTERN_MATCH(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15, N, ...)   N

// VA_NUM_ARGS
// copied from comments section of:
// http://efesx.com/2010/07/17/variadic-macro-to-count-number-of-arguments/
// which itself was derived from:
// https://gustedt.wordpress.com/2010/06/08/detect-empty-macro-arguments/
#define _ARG16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ...) _15
#define HAS_COMMA(...) _ARG16(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define HAS_NO_COMMA(...) _ARG16(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
#define _TRIGGER_PARENTHESIS_(...) ,

#define HAS_ZERO_OR_ONE_ARGS(...) \
    _HAS_ZERO_OR_ONE_ARGS( \
    /* test if there is just one argument, eventually an empty one */ \
    HAS_COMMA(__VA_ARGS__), \
    /* test if _TRIGGER_PARENTHESIS_ together with the argument adds a comma */ \
    HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__), \
    /* test if the argument together with a parenthesis adds a comma */ \
    HAS_COMMA(__VA_ARGS__ (~)), \
    /* test if placing it between _TRIGGER_PARENTHESIS_ and the parenthesis adds a comma */ \
    HAS_COMMA(_TRIGGER_PARENTHESIS_ __VA_ARGS__ (~)) \
    )

#define PASTE5(_0, _1, _2, _3, _4) _0 ## _1 ## _2 ## _3 ## _4
#define _HAS_ZERO_OR_ONE_ARGS(_0, _1, _2, _3) HAS_NO_COMMA(PASTE5(_IS_EMPTY_CASE_, _0, _1, _2, _3))
#define _IS_EMPTY_CASE_0001 ,

#define _VA0(...) HAS_ZERO_OR_ONE_ARGS(__VA_ARGS__)
#define _VA1(...) HAS_ZERO_OR_ONE_ARGS(__VA_ARGS__)
#define _VA2(...) 2
#define _VA3(...) 3
#define _VA4(...) 4
#define _VA5(...) 5
#define _VA6(...) 6
#define _VA7(...) 7
#define _VA8(...) 8
#define _VA9(...) 9
#define _VA10(...) 10
#define _VA11(...) 11
#define _VA12(...) 12
#define _VA13(...) 13
#define _VA14(...) 14
#define _VA15(...) 15
#define _VA16(...) 16

#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, PP_RSEQ_N(__VA_ARGS__) )
#define VA_NUM_ARGS_IMPL(...) VA_NUM_ARGS_N(__VA_ARGS__)

#define VA_NUM_ARGS_N( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
    _11,_12,_13,_14,_15,_16,N,...) N

#define PP_RSEQ_N(...) \
    _VA16(__VA_ARGS__),_VA15(__VA_ARGS__),_VA14(__VA_ARGS__),_VA13(__VA_ARGS__), \
    _VA12(__VA_ARGS__),_VA11(__VA_ARGS__),_VA10(__VA_ARGS__), _VA9(__VA_ARGS__), \
    _VA8(__VA_ARGS__),_VA7(__VA_ARGS__),_VA6(__VA_ARGS__),_VA5(__VA_ARGS__), \
    _VA4(__VA_ARGS__),_VA3(__VA_ARGS__),_VA2(__VA_ARGS__),_VA1(__VA_ARGS__), \
    _VA0(__VA_ARGS__)

//-----------------------------------------------------------------------------

#endif // MACROUTIL_H