
#include <algorithm> // for max

#include "config.hpp"

BENCHMARK_GLOBALS;

/**
 * Generic implementation.
 */
template<typename T>
void cmp(benchmark::input& input)
{
    T cur, prev;

    BENCHMARK_FOREACH(s)
    {
        cur = s;

        PUTCHAR('0' + (cur == prev));
        PUTCHAR('\n');

        prev = cur;
    }
}

#ifdef USE_PYTHON_STRING
/**
 * Python String Object specialization.
 */
template<>
void cmp<PyStringObject>(benchmark::input& input)
{
    PyObject *prev = PyString_FromStringAndSize("", 0);

    BENCHMARK_FOREACH(s)
    {
        PyObject *cur = PyString_FromString(s);
        PUTCHAR('0' + _PyString_Eq(cur, prev));
        PUTCHAR('\n');
        Py_DECREF(prev);
        prev = cur;
    }

    Py_DECREF(prev);
}
#endif // USE_PYTHON_STRING

#ifdef USE_PERL_STRING
/**
 * See util.c
 */
I32 Perl_foldEQ(const char *s1, const char *s2, register I32 len)
{
    register const U8 *a = (const U8 *)s1;
    register const U8 *b = (const U8 *)s2;

    while (len--)
    {
        if (*a != *b && *a != PL_fold[*b])
            return 0;
        a++,b++;
    }
    return 1;
}

/**
 * Perl scalar specialization.
 */
template<>
void cmp<SV>(benchmark::input& input)
{
    dTHX; /* fetch context */

    SV* prev = newSV(0);

    BENCHMARK_FOREACH(s)
    {
        SV* cur = newSVpv(s, 0);
        STRLEN s1_len, s2_len;
        const char* const s1 = SvPV(cur, s1_len);
        const char* const s2 = SvPV(prev, s2_len);
        PUTCHAR('0' + Perl_foldEQ(s1, s2, std::max(s1_len, s2_len)));
        PUTCHAR('\n');
        SvREFCNT_dec(prev);
        prev = cur;
    }
    SvREFCNT_dec(prev);
}
#endif // USE_PERL_STRING

#ifdef USE_GC_CORD
/**
 * GC CORD specialization.
 */
template<>
void cmp<CORD>(benchmark::input& input)
{
    CORD prev = CORD_EMPTY;

    BENCHMARK_FOREACH(s)
    {
        CORD cur = CORD_from_char_star(s);

        PUTCHAR('0' + (CORD_cmp(cur, prev) == 0));
        PUTCHAR('\n');

        prev = cur;
    }
}
#endif // USE_GC_CORD

int main(int argc, char* argv[])
{
    BENCHMARK_INIT;
    BENCHMARK_GET_ITERATIONS(iterations);
    BENCHMARK_ACQUIRE_INPUT(input);
    BENCHMARK_ITERATE(input, iterations)
    {
        cmp<STR>(input);
    }
    BENCHMARK_FINISH;
    return 0;
}
