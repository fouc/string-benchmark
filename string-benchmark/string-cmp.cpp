
#include "config.hpp"

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
    BENCHMARK_INPUT_ACQUIRE(data);
    cmp<STR>(data);
    return 0;
}
