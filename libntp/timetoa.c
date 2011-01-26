/*
 * timetoa.c -- time_t related string formatting
 *
 * Written by Juergen Perlinger (perlinger@ntp.org) for the NTP project.
 * The contents of 'html/copyright.html' apply.
 *
 * Printing a 'time_t' has a lot of portability pitfalls, due to it's
 * opaque base type. The only requirement imposed by the standard is
 * that it must be a numeric type. For all practical purposes it's a
 * signed int, and 32 bits are common.
 *
 * Since the UN*X time epoch will cause a signed integer overflow for
 * 32-bit signed int in the year 2038, implementations slowly move to
 * 64bit base types for time_t, even in 32-bit environments.
 *
 * As the printf() family has no standardised type specifier for time_t,
 * guessing the right output format specifier is a bit troublesome and
 * best done with the help of the preprocessor and "config.h".
 */

#include "config.h"

#include <math.h>
#include <stdio.h>

#include "timetoa.h"
#include "ntp_assert.h"
#include "lib_strbuf.h"

/*
 * Formatting to string needs at max 40 bytes (even with 64 bit time_t),
 * so we check LIB_BUFLENGTH is big enough for our purpose.
 */
#if LIB_BUFLENGTH < 40
# include "GRONK: LIB_BUFLENGTH is not sufficient"
#endif

/*
 * general fractional timestamp formatting
 *
 * Many pieces of ntpd require a machine with two's complement
 * representation of signed integers, so we don't go through the whole
 * rigamarole of creating fully portable code here. But we have to stay
 * away from signed integer overflow, as this might cause trouble even
 * with two's complement representation.
 */
const char *
format_time_fraction(
	time_t	secs,
	long	frac,
	int	prec
	)
{
	char *		cp;
	u_time		ttmp;	/* unsigned storage for seconds */
	int		i;
	long		fraclimit;
	int		notneg;	/* flag for non-negative value	*/
	const char *	fmt;
	ldiv_t		qr;

	DEBUG_REQUIRE(prec != 0);

	LIB_GETBUF(cp);
	ttmp = (u_time)secs;
	fmt = "-%" UTIME_FORMAT ".%0*ld";
	
	/* check if we need signed or unsigned mode */
	notneg = (prec < 0);
	prec = abs(prec);
	/* fraclimit = (long)pow(10, prec); */
	for (fraclimit = 10, i = 1; i < prec; i++)
		fraclimit *= 10;
	DEBUG_INSIST(fraclimit > 0);

	/*
	 * Since conversion to string uses lots of divisions anyway,
	 * there's no big extra penalty for normalisation. We do it for
	 * consistency.
	 */
	if (frac < 0 || frac >= fraclimit) {
		qr = ldiv(frac, fraclimit);
		if (qr.rem < 0) {
			qr.quot--;
			qr.rem += fraclimit;
		}
		ttmp += (time_t)qr.quot;
		frac = qr.rem;
	}

	/* Get the absolute value of the time stamp. */
	notneg = notneg || ((time_t)ttmp >= 0);
	if (notneg) {
		fmt++; /* skip sign char in format string */
	} else {
		ttmp = ~ttmp;
		if (frac != 0)
			frac = fraclimit - frac;
		else
			ttmp += 1;
	}

	/* finally format the data and return the result */
	snprintf(cp, LIB_BUFLENGTH, fmt, ttmp, prec, frac);
	
	return cp;
}
