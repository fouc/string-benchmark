
#include "config.hpp"

#ifdef USE_CONST_STRING
// provided for drop in compatibility with std::basic_string<>
#include "boost/const_string/concatenation.hpp"
#endif // USE_CONST_STRING

BENCHMARK_GLOBALS;

/**
 * Generic implementation.
 */
template<typename T>
unsigned long cat(benchmark::input& input)
{
    T res;

    BENCHMARK_FOREACH(s)
    {
        res += s;
    }

    return res.size();
}

#ifdef USE_PYTHON_STRING
/**
 * There's probably some BUGS here. But heh be fair to Python.
 *
 * See Python/ceval.c
 *
 * When Python encounters an expression of the form (x += y)
 * it compiles it down to an INPLACE_ADD opcode, if both x
 * and y are strings, this effectively bypasses the string API
 * PyString_Concat / PyString_ConcatAndDel methods.
 * INPLACE_ADD comes down to this function in ceval.c, which
 * tremendously improves performance when called on strings.
 */
static PyObject *
string_concatenate(PyObject *v, PyObject *w)
{
    /* This function implements 'variable += expr' when both arguments
       are strings. */
    Py_ssize_t v_len = PyString_GET_SIZE(v);
    Py_ssize_t w_len = PyString_GET_SIZE(w);
    Py_ssize_t new_len = v_len + w_len;
    if (new_len < 0)
    {
        PyErr_SetString(PyExc_OverflowError,
                        "strings are too large to concat");
        return NULL;
    }

    if (v->ob_refcnt == 1 && !PyString_CHECK_INTERNED(v))
    {
        /* Now we own the last reference to 'v', so we can resize it
         * in-place.
         */
        if (_PyString_Resize(&v, new_len) != 0)
        {
            /* XXX if _PyString_Resize() fails, 'v' has been
             * deallocated so it cannot be put back into
             * 'variable'.  The MemoryError is raised when there
             * is no value in 'variable', which might (very
             * remotely) be a cause of incompatibilities.
             */
            return NULL;
        }
        /* copy 'w' into the newly allocated area of 'v' */
        memcpy(PyString_AS_STRING(v) + v_len,
               PyString_AS_STRING(w), w_len);
        return v;
    }
    else
    {
        /* When in-place resizing is not an option. */
        PyString_Concat(&v, w);
        return v;
    }
}

/**
 * Python String Object specialization.
 */
template<>
unsigned long cat<PyStringObject>(benchmark::input& input)
{
    PyObject* res = PyString_FromStringAndSize("", 0);

    BENCHMARK_FOREACH(s)
    {
        PyObject* ns = PyString_FromString(s);
        if ((res = string_concatenate(res, ns)) == NULL)
        {
            exit(ECANCELED);
        }
        Py_DECREF(ns);
    }
    Py_ssize_t result = PyString_Size(res);
    Py_DECREF(res);
    return result;
}
#endif // USE_PYTHON_STRING

#ifdef USE_PERL_STRING
/**
 * Perl scalar specialization.
 */
template<>
unsigned long cat<SV>(benchmark::input& input)
{
    dTHX; /* fetch context */

    SV* res = newSV(0);

    BENCHMARK_FOREACH(s)
    {
        sv_catpv(res, s);
    }
    STRLEN result = SvCUR(res);
    SvREFCNT_dec(res);
    return result;
}
#endif // USE_PERL_STRING

#ifdef USE_GC_CORD
/**
 * GC CORD specialization.
 */
template<>
unsigned long cat<CORD>(benchmark::input& input)
{
    CORD res = CORD_EMPTY;

    BENCHMARK_FOREACH(s)
    {
        res = CORD_cat(res, CORD_from_char_star(s));
    }

    return CORD_len(res);
}
#endif // USE_GC_CORD

int main(int argc, char* argv[])
{
    BENCHMARK_INIT;
    BENCHMARK_GET_ITERATIONS(iterations);
    BENCHMARK_ACQUIRE_INPUT(input);
    BENCHMARK_ITERATE(input, iterations)
    {
        printf( "cat: %lu bytes.\n", cat<STR>(input));
    }
    BENCHMARK_FINISH;
    return 0;
}
