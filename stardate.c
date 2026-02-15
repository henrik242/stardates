/*
 *  stardate: convert between date formats
 *  by Andrew Main <zefram@fysh.org>
 *  1997-12-26, stardate [-30]0458.96
 *
 *  Stardate code is based on version 1 of the Stardates in Star Trek FAQ.
 */

/*
 * Copyright (c) 1996, 1997 Andrew Main.  All rights reserved.
 *
 * Redistribution and use, in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *        This product includes software developed by Andrew Main.
 * 4. The name of Andrew Main may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANDREW MAIN BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Unix programmers, please excuse the occasional DOSism in this code.
 *  DOS programmers, please excuse the Unixisms.  All programmers, please
 *  excuse the ANSIisms.  This program should actually run anywhere; I've
 *  tried to make it strictly conforming C.
 */

/*
 *  This program converts between dates in five formats:
 *    - stardates
 *    - the Julian calendar (with UTC time)
 *    - the Gregorian calendar (with UTC time)
 *    - the Quadcent calendar (see the Stardates FAQ for explanation)
 *    - traditional Unix time (seconds since 1970-01-01T00:00Z)
 *  Input and output can be in any of these formats.
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* for convenience (isxxx() want an unsigned char input) */

#define ISDIGIT(c) isdigit((unsigned char)(c))
#define ISALNUM(c) isalnum((unsigned char)(c))

/* Internal date format: an extended Unix-style date format.
 * This consists of the number of seconds since 0001=01=01 stored in a
 * 64 bit type, plus an additional 32 bit fraction of a second.
 * Each of the date formats used for I/O has a pair of functions,
 * used for converting from/to the internal format.
 */

typedef struct {
  uint64_t sec; /* seconds since 0001=01=01; unlimited range */
  uint32_t frac; /* range 0-(2^32-1) */
} intdate;

static void getcurdate(intdate *);
static void output(intdate const *);

static unsigned sdin(char const *, intdate *);
static unsigned julin(char const *, intdate *);
static unsigned gregin(char const *, intdate *);
static unsigned qcin(char const *, intdate *);
static unsigned unixin(char const *, intdate *);

static char const *sdout(intdate const *);
static char const *julout(intdate const *);
static char const *gregout(intdate const *);
static char const *qcout(intdate const *);
static char const *unixdout(intdate const *);
static char const *unixxout(intdate const *);

static struct format {
  char opt;
  bool sel;
  unsigned (*in)(char const *, intdate *);
  char const *(*out)(intdate const *);
} formats[] = {
  { 's', 0, sdin,   sdout    },
  { 'j', 0, julin,  julout   },
  { 'g', 0, gregin, gregout  },
  { 'q', 0, qcin,   qcout    },
  { 'u', 0, unixin, unixdout },
  { 'x', 0, NULL,   unixxout },
  { 0, 0, NULL, NULL }
};

static unsigned sddigits = 2;

static char const *progname;

int main(int argc, char **argv)
{
  struct format *f;
  bool sel = 0, haderr = 0;
  char *ptr;
  intdate dt;
  (void)argc;
  if((ptr = strrchr(*argv, '/')) || (ptr = strrchr(*argv, '\\')))
    progname = ptr+1;
  else
    progname = *argv;
  if(!*progname)
    progname = "stardate";
  while(*++argv && **argv == '-')
    while(*++*argv) {
      if(**argv == 'v') {
	printf("stardate 1.6.2\n");
	exit(EXIT_SUCCESS);
      }
      if(**argv == 'h') {
	printf("Usage: %s [-s[0-6]] [-j] [-g] [-q] [-u] [-x] [-h] [-v] [date ...]\n"
	       "Options:\n"
	       "  -s[N]  Output stardate (N = decimal digits, 0-6, default 2)\n"
	       "  -j     Output Julian calendar date\n"
	       "  -g     Output Gregorian calendar date\n"
	       "  -q     Output Quadcent calendar date\n"
	       "  -u     Output Unix time (decimal)\n"
	       "  -x     Output Unix time (hexadecimal)\n"
	       "  -h     Show this help\n"
	       "  -v     Show version\n"
	       "Input formats: [issue]number.frac, YYYY=MM=DD, YYYY-MM-DD, YYYY*MM*DD, Unumber\n",
	       progname);
	exit(EXIT_SUCCESS);
      }
      for(f = formats; f->opt; f++)
	if(**argv == f->opt) {
	  f->sel = sel = 1;
	  goto got;
	}
      fprintf(stderr, "%s: bad option: -%c\n", progname, **argv);
      exit(EXIT_FAILURE);
      got:
      if(**argv == 's' && argv[0][1] >= '0' && argv[0][1] <= '6')
	sddigits = *++*argv - '0';
    }
  if(!sel)
    formats[0].sel = 1;
  if(!*argv) {
    getcurdate(&dt);
    output(&dt);
  } else {
    do {
      unsigned n = 0;
      for(f = formats; f->opt; f++) {
	errno = 0;
	n = f->in ? f->in(*argv, &dt) : 0;
	if(n)
	  break;
      }
      haderr |= !(n & 1);
      if(!n)
	fprintf(stderr, "%s: date format unrecognised: %s\n", progname, *argv);
      else if(n == 1) {
	if(errno) {
	  fprintf(stderr, "%s: date is out of acceptable range: %s\n",
	      progname, *argv);
	  haderr = 1;
	} else
	  output(&dt);
      }
    } while(*++argv);
  }
  exit(haderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

static void getcurdate(intdate *dt)
{
  time_t t = time(NULL);
  struct tm *tm = gmtime(&t);
  char utc[20];
  sprintf(utc, "%04d-%02d-%02dT%02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1,
      tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  gregin(utc, dt);
}

static void output(intdate const *dt)
{
  struct format *f;
  bool d1 = 0;
  for(f = formats; f->opt; f++)
    if(f->sel) {
      if(d1)
	putchar(' ');
      d1 = 1;
      fputs(f->out(dt), stdout);
    }
  putchar('\n');
}

/* uint64str: convert a uint64_t to a string in the given radix with
 * at least `min` digits. */
static char const *uint64str(uint64_t n, unsigned radix, unsigned min)
{
  static char ret[21];
  char *pos = ret + 20;
  char *end = pos - min;
  *pos = 0;
  while(n) {
    *--pos = "0123456789abcdef"[n % radix];
    n /= radix;
  }
  while(pos > end)
    *--pos = '0';
  return pos;
}

/* The length of one quadcent year, 12622780800 / 400 == 31556952 seconds. */
#define QCYEAR 31556952UL
#define STDYEAR 31536000UL

/* Definitions to help with leap years. */
static unsigned nrmdays[12]={ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static unsigned lyrdays[12]={ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
#define jleapyear(y) ( !((y)%4L) )
#define gleapyear(y) ( !((y)%4L) && ( ((y)%100L) || !((y)%400L) ) )
#define jdays(y) (jleapyear(y) ? lyrdays : nrmdays)
#define gdays(y) (gleapyear(y) ? lyrdays : nrmdays)
#define xdays(gp, y) (((gp) ? gleapyear(y) : jleapyear(y)) ? lyrdays : nrmdays)

/* The date 0323-01-01 (0323*01*01) is 117609 days after the internal   *
 * epoch, 0001=01=01 (0000-12-30).  This is a difference of             *
 * 117609*86400 (0x1cb69*0x15180) == 10161417600 (0x25daaed80) seconds. */
static uint64_t const qcepoch = UINT64_C(0x25daaed80);

/* The length of four centuries, 146097 days of 86400 seconds, is *
 * 12622780800 (0x2f0605980) seconds.                             */
static uint64_t const quadcent = UINT64_C(0x2f0605980);

/* The epoch for Unix time, 1970-01-01, is 719164 (0xaf93c) days after *
 * our internal epoch, 0001=01=01 (0000-12-30).  This is a difference  *
 * of 719164*86400 (0xaf93c*0x15180) == 62135769600 (0xe77949a00)      *
 * seconds.                                                            */
static uint64_t const unixepoch = UINT64_C(0xe77949a00);

/* The epoch for stardates, 2162-01-04, is 789294 (0xc0b2e) days after *
 * the internal epoch.  This is 789294*86400 (0xc0b2e*0x15180) ==      *
 * 68195001600 (0xfe0bd2500) seconds.                                  */
static uint64_t const ufpepoch = UINT64_C(0xfe0bd2500);

/* The epoch for TNG-style stardates, 2323-01-01, is 848094 (0xcf0de) *
 * days after the internal epoch.  This is 73275321600 (0x110f8cad00) *
 * seconds.                                                           */
static uint64_t const tngepoch = UINT64_C(0x110f8cad00);

struct caldate {
  uint64_t year;
  unsigned month, day;
  unsigned hour, min, sec;
};
static unsigned readcal(struct caldate *, char const *, char);

static unsigned sdin(char const *date, intdate *dt)
{
  uint64_t nissue;
  uint32_t integer, frac;
  char const *cptr = date;
  char *ptr;
  int oerrno;
  bool negi;
  char fracbuf[7];
  if(*cptr++ != '[')
    return 0;
  negi = (*cptr == '-');
  if(negi)
    cptr++;
  if(!ISDIGIT(*cptr))
    return 0;
  errno = 0;
  nissue = strtoull(cptr, &ptr, 10);
  oerrno = errno;
  if(*ptr++ != ']' || !ISDIGIT(*ptr))
    return 0;
  integer = strtoul(ptr, &ptr, 10);
  if(errno || integer > 99999UL ||
      (!negi && nissue == 20 && integer > 5005UL) ||
      ((negi || nissue < 20) && integer > 9999UL)) {
    fprintf(stderr, "%s: integer part is out of range: %s\n", progname, date);
    return 2;
  }
  if(*ptr == '.') {
    char *b = fracbuf;
    strcpy(fracbuf, "000000");
    ptr++;
    while(*b && ISDIGIT(*ptr))
      *b++ = *ptr++;
    while(ISDIGIT(*ptr))
      ptr++;
    if(*ptr)
      return 0;
  } else if(*ptr)
    return 0;
  frac = strtoul(fracbuf, NULL, 10);
  errno = oerrno;
  if(negi || nissue <= 20) {
    /* Pre-TNG stardate */
    uint64_t f;
    if(!negi) {
      /* There are two changes in stardate rate to handle: *
       *       up to [19]7340      0.2 days/unit           *
       * [19]7340 to [19]7840     10   days/unit           *
       * [19]7840 to [20]5006      2   days/unit           *
       * we scale to the first of these.                   */
      if(nissue == 20) {
	nissue = 19;
	integer += 10000UL;
	goto fiddle;
      } else if(nissue == 19 && integer >= 7340UL) {
	fiddle:
	/* We have a stardate in the range [19]7340 to [19]15006.  First *
	 * we scale it to match the prior rate, so this range changes to *
	 * 7340 to 390640.                                               */
	integer = 7340UL + ((integer - 7340UL) * 50) + frac / (1000000UL/50);
	frac = (frac * 50UL) % 1000000UL;
	/* Next, if the stardate is greater than what was originally     *
	 * [19]7840 (now represented as 32340), it is in the 2 days/unit *
	 * range, so scale it back again.  The range affected, 32340 to  *
	 * 390640, changes to 32340 to 104000.                           */
	if(integer >= 32340UL) {
	  frac = frac/5UL + (integer%5UL) * (1000000UL/5);
	  integer = 32340UL + (integer - 32340UL) / 5;
	}
	/* The odd stardate has now been scaled to match the early stardate *
	 * type.  It could be up to [19]104000.  Fortunately this will not  *
	 * cause subsequent calculations to overflow.                       */
      }
      dt->sec = ufpepoch + nissue * (uint64_t)(2000UL*86400UL);
    } else {
      /* Negative stardate.  In order to avoid underflow in some cases, we *
       * actually calculate a date one issue (2000 days) too late, and     *
       * then subtract that much as the last stage.                        */
      dt->sec = ufpepoch - (nissue - 1) * (uint64_t)(2000UL*86400UL);
    }
    dt->sec += (uint64_t)(86400UL/5UL) * integer;
    /* frac is scaled such that it is in the range 0-999999, and a value *
     * of 1000000 would represent 86400/5 seconds.  We want to put frac  *
     * in the top half of a uint64, multiply by 86400/5 and divide by    *
     * 1000000, in order to leave the uint64 containing (top half) a     *
     * number of seconds and (bottom half) a fraction.  In order to      *
     * avoid overflow, this scaling is cancelled down to a multiply by   *
     * 54 and a divide by 3125.                                          */
    f = ((uint64_t)frac << 32) * 54UL;
    f = (f + 3124UL) / 3125UL;
    dt->sec += (uint32_t)(f >> 32);
    dt->frac = (uint32_t)f;
    if(negi) {
      /* Subtract off the issue that was added above. */
      dt->sec -= 2000UL*86400UL;
    }
  } else {
    uint64_t t;
    /* TNG stardate */
    nissue -= 21;
    /* Each issue is 86400*146097/4 seconds long. */
    dt->sec = tngepoch + nissue * (uint64_t)((86400UL/4UL)*146097UL);
    /* 1 unit is (86400*146097/4)/100000 seconds, which isn't even. *
     * It cancels to 27*146097/125.                                 */
    t = (uint64_t)integer * 1000000UL;
    t += frac;
    t *= 27UL*146097UL;
    dt->sec += t / 125000000UL;
    t = ((uint64_t)(t % 125000000UL) << 32);
    t = (t + 124999999UL) / 125000000UL;
    dt->frac = (uint32_t)t;
  }
  return 1;
}

static unsigned calin(char const *, intdate *, bool);

static unsigned julin(char const *date, intdate *dt)
{
  return calin(date, dt, 0);
}

static unsigned gregin(char const *date, intdate *dt)
{
  return calin(date, dt, 1);
}

static unsigned calin(char const *date, intdate *dt, bool gregp)
{
  struct caldate c;
  uint64_t t;
  bool low;
  unsigned cycle;
  unsigned n = readcal(&c, date, gregp ? '-' : '=');
  if(n != 1)
    return n;
  cycle = c.year % 400UL;
  if(c.day > xdays(gregp, cycle)[c.month - 1]) {
    fprintf(stderr, "%s: day is out of range: %s\n", progname, date);
    return 2;
  }
  low = (gregp && c.year == 0);
  if(low)
    c.year = 399;
  else
    c.year--;
  t = c.year * 365UL;
  if(gregp) {
    t -= c.year / 100UL;
    t += c.year / 400UL;
  }
  t += c.year / 4UL;
  n = 2*(unsigned)gregp + c.day - 1;
  for(c.month--; c.month--; )
    n += xdays(gregp, cycle)[c.month];
  t += n;
  if(low)
    t -= 146097UL;
  t *= 86400UL;
  dt->sec = t + (uint64_t)(c.hour*3600UL + c.min*60UL + c.sec);
  dt->frac = 0;
  return 1;
}

static unsigned qcin(char const *date, intdate *dt)
{
  struct caldate c;
  uint64_t secs, t, f;
  bool low;
  unsigned n = readcal(&c, date, '*');
  if(n != 1)
    return n;
  if(c.day > nrmdays[c.month - 1]) {
    fprintf(stderr, "%s: day is out of range: %s\n", progname, date);
    return 2;
  }
  low = (c.year < 323);
  if(low)
    c.year += 400UL - 323UL;
  else
    c.year -= 323;
  secs = qcepoch + c.year * (uint64_t)QCYEAR;
  for(n = c.day - 1, c.month--; c.month--; )
    n += nrmdays[c.month];
  t = (uint64_t)(n * 86400UL + c.hour * 3600UL + c.min * 60UL + c.sec);
  t *= QCYEAR;
  f = ((uint64_t)(t % STDYEAR) << 32) + STDYEAR - 1;
  secs += t / STDYEAR;
  if(low)
    secs -= quadcent;
  dt->sec = secs;
  dt->frac = (uint32_t)(f / STDYEAR);
  return 1;
}

static unsigned readcal(struct caldate *c, char const *date, char sep)
{
  int oerrno;
  unsigned long ul;
  char *ptr;
  char const *pos = date;
  if(!ISDIGIT(*pos))
    return 0;
  while(ISDIGIT(*++pos));
  if(*pos++ != sep || !ISDIGIT(*pos))
    return 0;
  while(ISDIGIT(*++pos));
  if(*pos++ != sep || !ISDIGIT(*pos))
    return 0;
  while(ISDIGIT(*++pos));
  if(*pos) {
    if((*pos != 'T' && *pos != 't') || !ISDIGIT(*++pos)) {
      badtime:
      fprintf(stderr, "%s: malformed time of day: %s\n", progname, date);
      return 2;
    }
    while(ISDIGIT(*++pos));
    if(*pos++ != ':' || !ISDIGIT(*pos))
      goto badtime;
    while(ISDIGIT(*++pos));
    if(*pos) {
      if(*pos++ != ':' || !ISDIGIT(*pos))
	goto badtime;
      while(ISDIGIT(*++pos));
      if(*pos)
	goto badtime;
    }
  }
  errno = 0;
  c->year = strtoull(date, &ptr, 10);
  oerrno = errno;
  errno = 0;
  ul = strtoul(ptr+1, &ptr, 10);
  if(errno || !ul || ul > 12UL) {
    fprintf(stderr, "%s: month is out of range: %s\n", progname, date);
    return 2;
  }
  c->month = ul;
  ul = strtoul(ptr+1, &ptr, 10);
  if(errno || !ul || ul > 31UL) {
    fprintf(stderr, "%s: day is out of range: %s\n", progname, date);
    return 2;
  }
  c->day = ul;
  if(!*ptr) {
    c->hour = c->min = c->sec = 0;
    errno = oerrno;
    return 1;
  }
  ul = strtoul(ptr+1, &ptr, 10);
  if(errno || ul > 23UL) {
    fprintf(stderr, "%s: hour is out of range: %s\n", progname, date);
    return 2;
  }
  c->hour = ul;
  ul = strtoul(ptr+1, &ptr, 10);
  if(errno || ul > 59UL) {
    fprintf(stderr, "%s: minute is out of range: %s\n", progname, date);
    return 2;
  }
  c->min = ul;
  if(!*ptr) {
    c->sec = 0;
    errno = oerrno;
    return 1;
  }
  ul = strtoul(ptr+1, &ptr, 10);
  if(errno || ul > 59UL) {
    fprintf(stderr, "%s: second is out of range: %s\n", progname, date);
    return 2;
  }
  c->sec = ul;
  errno = oerrno;
  return 1;
}

static unsigned unixin(char const *date, intdate *dt)
{
  char const *pos = date+1;
  unsigned radix = 10;
  bool neg;
  char *ptr;
  uint64_t mag;
  if(date[0] != 'u' && date[0] != 'U')
    return 0;
  neg = (*pos == '-');
  if(neg)
    pos++;
  if(pos[0] == '0' && (pos[1] == 'x' || pos[1] == 'X')) {
    pos += 2;
    radix = 16;
  }
  if(!ISALNUM(*pos)) {
    bad:
    fprintf(stderr, "%s: malformed Unix date: %s\n", progname, date);
    return 2;
  }
  mag = strtoull(pos, &ptr, radix);
  if(*ptr)
    goto bad;
  dt->sec = neg ? unixepoch - mag : unixepoch + mag;
  dt->frac = 0;
  return 1;
}

static char const *tngsdout(intdate const *);

static char const *sdout(intdate const *dt)
{
  bool isneg = 0;
  uint32_t nissue = 0, integer = 0;
  uint64_t frac;
  static char ret[18];
  if(tngepoch <= dt->sec)
    return tngsdout(dt);
  if(dt->sec < ufpepoch) {
    /* Negative stardate */
    uint64_t diff = ufpepoch - dt->sec - 1;
    uint32_t nsecs = 2000UL*86400UL - 1 - (uint32_t)(diff % (2000UL * 86400UL));
    isneg = 1;
    nissue = 1 + (uint32_t)(diff / (2000UL * 86400UL));
    integer = nsecs / (86400UL/5);
    frac = ((uint64_t)(nsecs % (86400UL/5)) << 32 | dt->frac) * 50UL;
  } else if(dt->sec < tngepoch) {
    /* Positive stardate */
    uint64_t diff = dt->sec - ufpepoch;
    uint32_t nsecs = (uint32_t)(diff % (2000UL * 86400UL));
    isneg = 0;
    nissue = (uint32_t)(diff / (2000UL * 86400UL));
    if(nissue < 19 || (nissue == 19 && nsecs < 7340UL * (86400UL/5))) {
      /* TOS era */
      integer = nsecs / (86400UL/5);
      frac = ((uint64_t)(nsecs % (86400UL/5)) << 32 | dt->frac) * 50UL;
    } else {
      /* Film era */
      nsecs += (nissue - 19) * 2000UL*86400UL;
      nissue = 19;
      nsecs -= 7340UL * (86400UL/5);
      if(nsecs >= 5000UL*86400UL) {
	/* Late film era */
	nsecs -= 5000UL*86400UL;
	integer = 7840 + nsecs/(86400UL*2);
	if(integer >= 10000) {
	  integer -= 10000;
	  nissue++;
	}
	frac = ((uint64_t)(nsecs % (86400UL*2)) << 32 | dt->frac) * 5UL;
      } else {
	/* Early film era */
	integer = 7340 + nsecs/(86400UL*10);
	frac = (uint64_t)(nsecs % (86400UL*10)) << 32 | dt->frac;
      }
    }
  }
  sprintf(ret, "[%s%lu]%04lu", isneg ? "-" : "", (unsigned long)nissue, (unsigned long)integer);
  if(sddigits) {
    char *ptr = strchr(ret, 0);
    /* At this point, frac is a fractional part of a unit, in the range *
     * 0 to (2^32 * 864000)-1.  In order to represent this as a 6-digit *
     * decimal fraction, we need to scale this.  Mathematically, we     *
     * need to multiply by 1000000 and divide by (2^32 * 864000).  But  *
     * multiplying by 1000000 would cause overflow.  Cancelling the two *
     * values yields an algorithm of multiplying by 125 and dividing by *
     * (2^32*108).                                                      */
    frac = frac * 125UL / 108UL;
    sprintf(ptr, ".%06lu", (unsigned long)(uint32_t)(frac >> 32));
    ptr[sddigits + 1] = 0;
  }
  return ret;
}

static char const *tngsdout(intdate const *dt)
{
  static char ret[36];
  uint64_t h, l;
  uint32_t nsecs;
  uint64_t diff = dt->sec - tngepoch;
  /* 1 issue is 86400*146097/4 seconds long, which just fits in 32 bits. */
  uint64_t nissue = 21 + diff / ((86400UL/4)*146097UL);
  nsecs = (uint32_t)(diff % ((86400UL/4)*146097UL));
  /* 1 unit is (86400*146097/4)/100000 seconds, which isn't even. *
   * It cancels to 27*146097/125.  For a six-figure fraction,     *
   * divide that by 1000000.                                      */
  h = (uint64_t)nsecs * 125000000UL;
  l = (uint64_t)dt->frac * 125000000UL;
  h += (uint32_t)(l >> 32);
  h /= (27UL*146097UL);
  sprintf(ret, "[%s]%05lu", uint64str(nissue, 10, 1),
      (unsigned long)(uint32_t)(h / 1000000UL));
  if(sddigits) {
    char *ptr = strchr(ret, 0);
    sprintf(ptr, ".%06lu", (unsigned long)(uint32_t)(h % 1000000UL));
    ptr[sddigits + 1] = 0;
  }
  return ret;
}

static char const *calout(intdate const *, bool);

static char const *julout(intdate const *dt)
{
  return calout(dt, 0);
}

static char const *gregout(intdate const *dt)
{
  return calout(dt, 1);
}

static char const *docalout(char, bool, unsigned, uint64_t, unsigned, uint32_t);

static char const *calout(intdate const *dt, bool gregp)
{
  uint32_t tod = (uint32_t)(dt->sec % 86400UL);
  uint64_t year, days = dt->sec / 86400UL;
  /* We need the days number to be days since an xx01.01.01 to get the *
   * leap year cycle right.  For the Julian calendar, it is already    *
   * so (0001=01=01).  But for the Gregorian calendar, the epoch is    *
   * 0000-12-30, so we must add on 400 years minus 2 days.  The year   *
   * number gets corrected below.                                      */
  if(gregp)
    days += 146095UL;
  /* Approximate the year number, underestimating but only by a limited *
   * amount.  days/366 is a first approximation, but it goes out by 1   *
   * day every non-leap year, and so will be a full year out after 366  *
   * non-leap years.  In the Julian calendar, we get 366 non-leap years *
   * every 488 years, so adding (days/366)/487 corrects for this.  In   *
   * the Gregorian calendar, it is not so simple: we get 400 years      *
   * every 146097 days, and then add on days/366 within that set of 400 *
   * years.                                                             */
  if(gregp)
    year = (days / 146097UL) * 400UL + (days % 146097UL) / 366UL;
  else
    year = days / 366UL + days / (366UL * 487UL);
  /* We then adjust the number of days remaining to match this *
   * approximation of the year.  Note that this approximation  *
   * will never be more than two years off the correct date,   *
   * so the number of days left no longer needs to be stored   *
   * in a uint64.                                              */
  if(gregp)
    days = days + year / 100UL - year / 400UL;
  days -= year * 365UL + year / 4UL;
  /* Now correct the year to an actual year number (see notes above). */
  if(gregp)
    year -= 399;
  else
    year++;
  return docalout(gregp ? '-' : '=', gregp, (unsigned)(year % 400UL),
      year, (unsigned)days, tod);
}

static char const *docalout(char sep, bool gregp, unsigned cycle,
    uint64_t year, unsigned ndays, uint32_t tod)
{
  unsigned nmonth = 0;
  unsigned hr, min, sec;
  static char ret[36];
  /* Walk through the months, fixing the year, and as a side effect *
   * calculating the month number and day of the month.             */
  while(ndays >= xdays(gregp, cycle)[nmonth]) {
    ndays -= xdays(gregp, cycle)[nmonth];
    if(++nmonth == 12) {
      nmonth = 0;
      year++;
      cycle++;
    }
  }
  ndays++;
  nmonth++;
  /* Now sort out the time of day. */
  hr = tod / 3600;
  tod %= 3600;
  min = tod / 60;
  sec = tod % 60;
  sprintf(ret, "%s%c%02d%c%02dT%02d:%02d:%02d",
      uint64str(year, 10, 4), sep, nmonth, sep, ndays, hr, min, sec);
  return ret;
}

static char const *qcout(intdate const *dt)
{
  uint64_t secs = dt->sec;
  uint32_t nsec;
  uint64_t year, h, l;
  bool low;
  low = (secs < qcepoch);
  if(low)
    secs += quadcent;
  secs -= qcepoch;
  nsec = (uint32_t)(secs % QCYEAR);
  secs /= QCYEAR;
  if(low)
    year = secs - (400 - 323);
  else
    year = secs + 323;
  /* We need to translate the nsec:dt->frac value (real seconds up to *
   * 31556952:0) into quadcent seconds.  This can be done by          *
   * multiplying by 146000 and dividing by 146097.  Normally this     *
   * would overflow, so we do this in two parts.                      */
  h = (uint64_t)nsec * 146000UL;
  l = (uint64_t)dt->frac * 146000UL;
  h += (uint32_t)(l >> 32);
  nsec = (uint32_t)(h / 146097UL);
  return docalout('*', 0, 1, year, nsec / 86400, nsec % 86400UL);
}

static char const *unixout(intdate const *, unsigned, char const *);

static char const *unixdout(intdate const *dt)
{
  return unixout(dt, 10, "");
}

static char const *unixxout(intdate const *dt)
{
  return unixout(dt, 16, "0x");
}

static char const *unixout(intdate const *dt, unsigned radix, char const *prefix)
{
  static char ret[24];
  char const *sgn;
  uint64_t mag;
  if(unixepoch <= dt->sec) {
    sgn = "";
    mag = dt->sec - unixepoch;
  } else {
    sgn = "-";
    mag = unixepoch - dt->sec;
  }
  sprintf(ret, "U%s%s%s", sgn, prefix, uint64str(mag, radix, 1));
  return ret;
}
