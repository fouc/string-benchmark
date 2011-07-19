
#include <algorithm> // for max

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

#ifdef USE_PERL_STRING
/**
 * See sv.c
 */
I32
Perl_sv_eq_flags(pTHX_ register SV *sv1, register SV *sv2, const U32 flags)
{
    dVAR;
    const char *pv1;
    STRLEN cur1;
    const char *pv2;
    STRLEN cur2;
    I32  eq     = 0;
    char *tpv   = NULL;
    SV* svrecode = NULL;

    if (!sv1)
    {
        pv1 = "";
        cur1 = 0;
    }
    else
    {
        /* if pv1 and pv2 are the same, second SvPV_const call may
         * invalidate pv1 (if we are handling magic), so we may need to
         * make a copy */
        if (sv1 == sv2 && flags & SV_GMAGIC
                && (SvTHINKFIRST(sv1) || SvGMAGICAL(sv1)))
        {
            pv1 = SvPV_const(sv1, cur1);
            sv1 = newSVpvn_flags(pv1, cur1, SVs_TEMP | SvUTF8(sv2));
        }
        pv1 = SvPV_flags_const(sv1, cur1, flags);
    }

    if (!sv2)
    {
        pv2 = "";
        cur2 = 0;
    }
    else
        pv2 = SvPV_flags_const(sv2, cur2, flags);

    if (cur1 && cur2 && SvUTF8(sv1) != SvUTF8(sv2) && !IN_BYTES)
    {
        /* Differing utf8ness.
        * Do not UTF8size the comparands as a side-effect. */
        if (PL_encoding)
        {
            if (SvUTF8(sv1))
            {
                svrecode = newSVpvn(pv2, cur2);
                sv_recode_to_utf8(svrecode, PL_encoding);
                pv2 = SvPV_const(svrecode, cur2);
            }
            else
            {
                svrecode = newSVpvn(pv1, cur1);
                sv_recode_to_utf8(svrecode, PL_encoding);
                pv1 = SvPV_const(svrecode, cur1);
            }
            /* Now both are in UTF-8. */
            if (cur1 != cur2)
            {
                SvREFCNT_dec(svrecode);
                return FALSE;
            }
        }
        else
        {
            if (SvUTF8(sv1))
            {
                /* sv1 is the UTF-8 one  */
                exit(-1); // Not handled
            }
            else
            {
                /* sv2 is the UTF-8 one  */
                exit(-1); // Not handled
            }
        }
    }

    if (cur1 == cur2)
        eq = (pv1 == pv2) || memEQ(pv1, pv2, cur1);

    SvREFCNT_dec(svrecode);
    if (tpv)
        Safefree(tpv);

    return eq;
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
        PUTCHAR('0' + Perl_sv_eq_flags(aTHX_ cur, prev, SV_GMAGIC));
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
