
#include "config.hpp"

#include <algorithm>
#include <cstring>

/**
 * Generic implementation.
 */
template<typename T>
unsigned long slice(benchmark::input& input)
{
    size_t total = 0, prev = 0, ante = 0;

    BENCHMARK_FOREACH(s)
    {
        T str(s);

        size_t cur = str.size();
        if (cur != 0)
        {
            size_t from = prev % cur;
            size_t to   = ante % cur;

            if (from > to)
            {
                std::swap(from, to);
            }

            T sliced = str.substr(from, (to - from));

            total += sliced.size();
        }
        ante = prev;
        prev = cur;
    }

    return total;
}

#ifdef USE_PYTHON_STRING
/**
 * Python String Object specialization.
 */
template<>
unsigned long slice<PyStringObject>(benchmark::input& input)
{
    size_t total = 0, prev = 0, ante = 0;

    BENCHMARK_FOREACH(s)
    {
        PyObject *str = PyString_FromString(s);

        size_t cur = PyString_Size(str);
        if (cur != 0)
        {
            size_t from = prev % cur;
            size_t to   = ante % cur;

            if (from > to)
            {
                std::swap(from, to);
            }

            PyObject* sliced = PySequence_GetSlice(str, from, to);

            total += PyString_Size(sliced);

            Py_DECREF(sliced);
        }
        ante = prev;
        prev = cur;

        Py_DECREF(str);
    }

    return total;
}
#endif // USE_PYTHON_STRING

#ifdef USE_PERL_STRING
/**
 * See mg.c
 * This is substr (as rvalue).
 */
int Perl_magic_getsubstr(pTHX_ SV *sv, MAGIC *)
{
    STRLEN len;
    SV * const lsv = LvTARG(sv);
    const char * const tmps = SvPV_const(lsv,len);
    STRLEN offs = LvTARGOFF(sv);
    STRLEN rem = LvTARGLEN(sv);

    if (SvUTF8(lsv))
        exit(-1); // Not handled
    if (offs > len)
        offs = len;
    if (rem > len - offs)
        rem = len - offs;
    sv_setpvn(sv, tmps + offs, rem);
    if (SvUTF8(lsv))
        SvUTF8_on(sv);
    return 0;
}

/**
 * Perl scalar specialization.
 * Emulates slicing with a pseudo call to substr.
 */
template<>
unsigned long slice<SV>(benchmark::input& input)
{
    dTHX; /* fetch context */

    size_t total = 0, prev = 0, ante = 0;

    SV* const slice_args = sv_2mortal(newSV_type(SVt_PVLV));

    BENCHMARK_FOREACH(s)
    {
        SV* str = newSVpv(s, 0);

        size_t cur = SvCUR(str);
        if (cur != 0)
        {
            size_t from = prev % cur;
            size_t to   = ante % cur;

            if (from > to)
            {
                std::swap(from, to);
            }

            LvTARG(slice_args) = SvREFCNT_inc_simple(str);
            LvTARGOFF(slice_args) = from;
            LvTARGLEN(slice_args) = (to - from);
            Perl_magic_getsubstr(aTHX_ slice_args, 0);

            total += SvCUR(slice_args);
        }
        ante = prev;
        prev = cur;

        SvREFCNT_dec(str);
    }

    return total;
}
#endif // USE_PERL_STRING

#ifdef USE_GC_CORD
/**
 * GC CORD specialization.
 */
template<>
unsigned long slice<CORD>(benchmark::input& input)
{
    size_t total = 0, prev = 0, ante = 0;

    BENCHMARK_FOREACH(s)
    {
        CORD str = CORD_from_char_star(s);

        size_t cur = CORD_len(str);
        if (cur != 0)
        {
            size_t from = prev % cur;
            size_t to   = ante % cur;

            if (from > to)
            {
                std::swap(from, to);
            }

            CORD sliced = CORD_substr(str, from, (to - from));

            total += CORD_len(sliced);
        }
        ante = prev;
        prev = cur;
    }

    return total;
}
#endif // USE_GC_CORD

int main(int argc, char* argv[])
{
    BENCHMARK_INIT;
    BENCHMARK_GET_ITERATIONS(iterations);
    BENCHMARK_ACQUIRE_INPUT(input);
    BENCHMARK_ITERATE(input, iterations)
    {
        printf( "slice: %lu bytes.\n", slice<STR>(input));
    }
    BENCHMARK_FINISH;
    return 0;
}
