/*    deb.c
 *
 *    Copyright (c) 1991-1997, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/*
 * "Didst thou think that the eyes of the White Tower were blind?  Nay, I
 * have seen more than thou knowest, Gray Fool."  --Denethor
 */

#include "EXTERN.h"
#include "perl.h"

#if !defined(I_STDARG) && !defined(I_VARARGS)

/*
 * Fallback on the old hackers way of doing varargs
 */

/*VARARGS1*/
void
deb(pat,a1,a2,a3,a4,a5,a6,a7,a8)
    char *pat;
{
#ifdef DEBUGGING
    dTHR;
    register I32 i;
    GV* gv = curcop->cop_filegv;

#ifdef USE_THREADS
    PerlIO_printf(Perl_debug_log,"0x%lx (%s:%ld)\t",
		  (unsigned long) thr,
		  SvTYPE(gv) == SVt_PVGV ? SvPVX(GvSV(gv)) : "<free>",
		  (long)curcop->cop_line);
#else
    PerlIO_printf(Perl_debug_log, "(%s:%ld)\t",
	SvTYPE(gv) == SVt_PVGV ? SvPVX(GvSV(gv)) : "<free>",
	(long)curcop->cop_line);
#endif /* USE_THREADS */
    for (i=0; i<dlevel; i++)
	PerlIO_printf(Perl_debug_log, "%c%c ",debname[i],debdelim[i]);
    PerlIO_printf(Perl_debug_log, pat,a1,a2,a3,a4,a5,a6,a7,a8);
#endif /* DEBUGGING */
}

#else /* !defined(I_STDARG) && !defined(I_VARARGS) */

#  ifdef I_STDARG
void
deb(const char *pat, ...)
#  else
/*VARARGS1*/
void
deb(pat, va_alist)
    const char *pat;
    va_dcl
#  endif
{
#ifdef DEBUGGING
    dTHR;
    va_list args;
    register I32 i;
    GV* gv = curcop->cop_filegv;

#ifdef USE_THREADS
    PerlIO_printf(Perl_debug_log, "0x%lx (%s:%ld)\t",
		  (unsigned long) thr,
		  SvTYPE(gv) == SVt_PVGV ? SvPVX(GvSV(gv)) : "<free>",
		  (long)curcop->cop_line);
#else
    PerlIO_printf(Perl_debug_log, "(%s:%ld)\t",
	SvTYPE(gv) == SVt_PVGV ? SvPVX(GvSV(gv)) : "<free>",
	(long)curcop->cop_line);
#endif /* USE_THREADS */
    for (i=0; i<dlevel; i++)
	PerlIO_printf(Perl_debug_log, "%c%c ",debname[i],debdelim[i]);

#  ifdef I_STDARG
    va_start(args, pat);
#  else
    va_start(args);
#  endif
    (void) PerlIO_vprintf(Perl_debug_log,pat,args);
    va_end( args );
#endif /* DEBUGGING */
}
#endif /* !defined(I_STDARG) && !defined(I_VARARGS) */

void
deb_growlevel(void)
{
#ifdef DEBUGGING
    dlmax += 128;
    Renew(debname, dlmax, char);
    Renew(debdelim, dlmax, char);
#endif /* DEBUGGING */
}

I32
debstackptrs(void)
{
#ifdef DEBUGGING
    dTHR;
    PerlIO_printf(Perl_debug_log, "%8lx %8lx %8ld %8ld %8ld\n",
	(unsigned long)curstack, (unsigned long)stack_base,
	(long)*markstack_ptr, (long)(stack_sp-stack_base),
	(long)(stack_max-stack_base));
    PerlIO_printf(Perl_debug_log, "%8lx %8lx %8ld %8ld %8ld\n",
	(unsigned long)mainstack, (unsigned long)AvARRAY(curstack),
	(long)mainstack, (long)AvFILLp(curstack), (long)AvMAX(curstack));
#endif /* DEBUGGING */
    return 0;
}

I32
debstack(void)
{
#ifdef DEBUGGING
    dTHR;
    I32 top = stack_sp - stack_base;
    register I32 i = top - 30;
    I32 *markscan = curstackinfo->si_markbase;

    if (i < 0)
	i = 0;
    
    while (++markscan <= markstack_ptr)
	if (*markscan >= i)
	    break;

#ifdef USE_THREADS
    PerlIO_printf(Perl_debug_log, i ? "0x%lx    =>  ...  " : "0x%lx    =>  ",
		  (unsigned long) thr);
#else
    PerlIO_printf(Perl_debug_log, i ? "    =>  ...  " : "    =>  ");
#endif /* USE_THREADS */
    if (stack_base[0] != &sv_undef || stack_sp < stack_base)
	PerlIO_printf(Perl_debug_log, " [STACK UNDERFLOW!!!]\n");
    do {
	++i;
	if (markscan <= markstack_ptr && *markscan < i) {
	    do {
		++markscan;
		PerlIO_putc(Perl_debug_log, '*');
	    }
	    while (markscan <= markstack_ptr && *markscan < i);
	    PerlIO_printf(Perl_debug_log, "  ");
	}
	if (i > top)
	    break;
	PerlIO_printf(Perl_debug_log, "%-4s  ", SvPEEK(stack_base[i]));
    }
    while (1);
    PerlIO_printf(Perl_debug_log, "\n");
#endif /* DEBUGGING */
    return 0;
}
