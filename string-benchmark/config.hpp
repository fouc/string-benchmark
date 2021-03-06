
/* Each benchmark must define an appropriate STR type. */

#ifdef USE_STD_STRING
#include <string>
typedef std::string STR;
#endif // USE_STD_STRING

#ifdef USE_STD_STRING_GC
#include <string>
#include <gc/gc_allocator.h>
typedef std::basic_string< char, std::char_traits<char>, gc_allocator<char> > STR;
#endif // USE_STD_STRING_GC

#ifdef USE_PYTHON_STRING
#include <Python.h>
typedef PyStringObject STR;
#endif // USE_PYTHON_STRING

#ifdef USE_PERL_STRING
#define PERL_NO_GET_CONTEXT /* we want efficiency */
#include <EXTERN.h>
#include <perl.h>
typedef SV STR;
#define BENCHMARK_INIT    static PerlInterpreter *my_perl; \
                          PERL_SYS_INIT(&argc, &argv);     \
                          my_perl = perl_alloc();          \
                          perl_construct(my_perl);
#define BENCHMARK_FINISH  perl_destruct(my_perl); \
                          perl_free(my_perl);     \
                          PERL_SYS_TERM();
#endif // USE_PERL_STRING

#ifdef USE_EXT_ROPE
#include <ext/rope>
typedef __gnu_cxx::rope<char> STR;
#endif // USE_EXT_ROPE

#ifdef USE_GC_CORD
extern "C"
{
#include <gc/cord.h>
}
typedef CORD STR;
#endif // USE_GC_CORD

#ifdef USE_CONST_STRING
#include "boost/const_string/const_string.hpp"
typedef boost::const_string<char> STR;
#endif // USE_CONST_STRING

#ifdef USE_BSTRLIB
#include "bstrwrap.h"
#define size   length
#define substr midstr
typedef Bstrlib::CBString STR;
#endif // USE_BSTRLIB

#ifdef USE_QT4_STRING
#include "QString"
#define substr mid // may be more efficient with midRef (TODO: specialize the template to be fair)
typedef QString STR;
#endif // USE_QT4_STRING

#include <cstddef> // for size_t (defined here to avoid a clash with Python.h)

#ifdef USE_NOTHING
/**
 * Null string class.
 */
struct NullString
{
    NullString() {}

    template<typename T>
    NullString(T) {}

    template<typename T>
    inline void operator+=(T) {}

    template<typename T>
    inline void operator=(T) {}

    template<typename T>
    inline bool operator==(T) const
    {
        return false;
    }

    inline size_t size() const
    {
        return 0;
    }

    inline const NullString& substr(size_t, size_t) const
    {
        return (*this);
    }
};
typedef NullString STR;
#endif // USE_NOTHING

#include "input.hpp"

#ifndef BENCHMARK_INIT
#define BENCHMARK_INIT
#endif // BENCHMARK_INIT

#ifndef BENCHMARK_FINISH
#define BENCHMARK_FINISH
#endif // BENCHMARK_FINISH
