#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/*
 * Contributed by Graham Barr  <Graham.Barr@tiuk.ti.com>
 * The main guts of traverse_isa was actually copied from gv_fetchmeth
 */

static SV *
isa_lookup(stash, name, len, level)
HV *stash;
char *name;
int len;
int level;
{
    AV* av;
    GV* gv;
    GV** gvp;
    HV* hv = Nullhv;

    if (!stash)
	return &sv_undef;

    if(strEQ(HvNAME(stash), name))
	return &sv_yes;

    if (level > 100)
	croak("Recursive inheritance detected");

    gvp = (GV**)hv_fetch(stash, "::ISA::CACHE::", 14, FALSE);

    if (gvp && (gv = *gvp) != (GV*)&sv_undef && (hv = GvHV(gv))) {
	SV* sv;
	SV** svp = (SV**)hv_fetch(hv, name, len, FALSE);
	if (svp && (sv = *svp) != (SV*)&sv_undef)
	    return sv;
    }

    gvp = (GV**)hv_fetch(stash,"ISA",3,FALSE);
    
    if (gvp && (gv = *gvp) != (GV*)&sv_undef && (av = GvAV(gv))) {
	if(!hv) {
	    gvp = (GV**)hv_fetch(stash, "::ISA::CACHE::", 14, TRUE);

	    gv = *gvp;

	    if (SvTYPE(gv) != SVt_PVGV)
		gv_init(gv, stash, "::ISA::CACHE::", 14, TRUE);

	    hv = GvHVn(gv);
	}
	if(hv) {
	    SV** svp = AvARRAY(av);
	    I32 items = AvFILL(av) + 1;
	    while (items--) {
		SV* sv = *svp++;
		HV* basestash = gv_stashsv(sv, FALSE);
		if (!basestash) {
		    if (dowarn)
			warn("Can't locate package %s for @%s::ISA",
			    SvPVX(sv), HvNAME(stash));
		    continue;
		}
		if(&sv_yes == isa_lookup(basestash, name, len, level + 1)) {
		    (void)hv_store(hv,name,len,&sv_yes,0);
		    return &sv_yes;
		}
	    }
	    (void)hv_store(hv,name,len,&sv_no,0);
	}
    }

    return &sv_no;
}

static
XS(XS_UNIVERSAL_isa)
{
    dXSARGS;
    SV *sv, *rv;
    char *name;

    if (items != 2)
	croak("Usage: UNIVERSAL::isa(reference, kind)");

    sv = ST(0);
    name = (char *)SvPV(ST(1),na);

    if (!SvROK(sv)) {
        rv = &sv_no;
    }
    else if((sv = (SV*)SvRV(sv)) && SvOBJECT(sv) &&
     &sv_yes == isa_lookup(SvSTASH(sv), name, strlen(name), 0)) {
	rv = &sv_yes;
    }
    else {
        char *s;

        switch (SvTYPE(sv)) {
            case SVt_NULL:
    	    case SVt_IV:
	    case SVt_NV:
	    case SVt_RV:
	    case SVt_PV:
	    case SVt_PVIV:
	    case SVt_PVNV:
	    case SVt_PVBM:
	    case SVt_PVMG:  s = "SCALAR";       break;
	    case SVt_PVLV:  s = "LVALUE";       break;
	    case SVt_PVAV:  s = "ARRAY";        break;
	    case SVt_PVHV:  s = "HASH";         break;
	    case SVt_PVCV:  s = "CODE";         break;
	    case SVt_PVGV:  s = "GLOB";         break;
	    case SVt_PVFM:  s = "FORMATLINE";   break;
	    case SVt_PVIO:  s = "FILEHANDLE";   break;
	    default:        s = "UNKNOWN";      break;
        }
        rv = strEQ(s,name) ? &sv_yes : &sv_no;
    }

    ST(0) = rv;
    XSRETURN(1);
}

static
XS(XS_UNIVERSAL_can)
{
    dXSARGS;
    SV   *sv;
    char *name;
    SV   *rv;
    GV   *gv;
    CV   *cvp;

    if (items != 2)
	croak("Usage: UNIVERSAL::can(object-ref, method)");

    sv = ST(0);
    name = (char *)SvPV(ST(1),na);
    rv = &sv_undef;

    if(SvROK(sv) && (sv = (SV*)SvRV(sv)) && SvOBJECT(sv)) {
        gv = gv_fetchmethod(SvSTASH(sv), name);

        if(gv && GvCV(gv)) {
            /* If the sub is only a stub then we may have a gv to AUTOLOAD */
            GV **gvp = (GV**)hv_fetch(GvSTASH(gv), name, strlen(name), TRUE);
            if(gvp && (cvp = GvCV(*gvp))) {
                rv = sv_newmortal();
                sv_setsv(rv, newRV((SV*)cvp));
            }
        }
    }

    ST(0) = rv;
    XSRETURN(1);
}

static
XS(XS_UNIVERSAL_is_instance)
{
    dXSARGS;
    ST(0) = SvROK(ST(0)) ? &sv_yes : &sv_no;
    XSRETURN(1);
}

static
XS(XS_UNIVERSAL_class)
{
    dXSARGS;
    if(SvROK(ST(0))) {
        SV *sv = sv_newmortal();
        sv_setpv(sv, HvNAME(SvSTASH(ST(0))));
        ST(0) = sv;
    }
    XSRETURN(1);
}

static
XS(XS_UNIVERSAL_VERSION)
{
    dXSARGS;
    HV *pkg;
    GV **gvp;
    GV *gv;
    SV *sv;
    char *undef;

    if(SvROK(ST(0))) {
        sv = (SV*)SvRV(ST(0));
        if(!SvOBJECT(sv))
            croak("Cannot find version of an unblessed reference");
        pkg = SvSTASH(sv);
    }
    else {
        pkg = gv_stashsv(ST(0), FALSE);
    }

    gvp = pkg ? (GV**)hv_fetch(pkg,"VERSION",7,FALSE) : Null(GV**);

    if (gvp && (gv = *gvp) != (GV*)&sv_undef && (sv = GvSV(gv))) {
        SV *nsv = sv_newmortal();
        sv_setsv(nsv, sv);
        sv = nsv;
        undef = Nullch;
    }
    else {
        sv = (SV*)&sv_undef;
        undef = "(undef)";
    }

    if(items > 1 && (undef || SvNV(ST(1)) > SvNV(sv)))
	croak("%s version %s required--this is only version %s",
	    HvNAME(pkg),SvPV(ST(1),na),undef ? undef : SvPV(sv,na));

    ST(0) = sv;

    XSRETURN(1);
}

void
boot_core_UNIVERSAL()
{
    char *file = __FILE__;

    newXS("UNIVERSAL::isa",             XS_UNIVERSAL_isa,         file);
    newXS("UNIVERSAL::can",             XS_UNIVERSAL_can,         file);
    newXS("UNIVERSAL::class",           XS_UNIVERSAL_class,       file);
    newXS("UNIVERSAL::is_instance",     XS_UNIVERSAL_is_instance, file);
    newXS("UNIVERSAL::VERSION", 	XS_UNIVERSAL_VERSION, 	  file);
}
