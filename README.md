# stardate

Convert between stardates and other calendar systems.

Originally written by Andrew Main (Zefram) in 1997, based on his
*Stardates in Star Trek FAQ*. See [README.orig](README.orig) for the
original documentation.

## Build

    make

## Usage

    stardate [options] [date ...]

With no arguments, prints the current time as a stardate.

### Output options

| Flag | Format |
|------|--------|
| `-s[0-6]` | Issue-based stardate from the Stardates FAQ (`[21]41000.15`) |
| `-n[0-6]` | TNG-style stardate, 1000 units/year (`41000.00`) |
| `-j` | Julian calendar date |
| `-g` | Gregorian calendar date |
| `-q` | Quadcent calendar date |
| `-u` | Unix time (decimal) |
| `-x` | Unix time (hex) |
| `-h` | Help |
| `-v` | Version |

The optional digit after `-s` or `-n` sets decimal precision (0-6, default 2).

### Input formats

| Format | Example |
|--------|---------|
| `[issue]number.frac` | `[23]4906.5` (issue-based stardate) |
| `number.frac` | `41153.7` (TNG-style stardate) |
| `YYYY-MM-DD[Thh:mm:ss]` | `2364-01-01` (Gregorian) |
| `YYYY=MM=DD[Thh:mm:ss]` | `2364=01=01` (Julian) |
| `YYYY*MM*DD[Thh:mm:ss]` | `2364*01*01` (Quadcent) |
| `Unumber` | `U1705276800` (Unix time) |

### Examples

    $ stardate
    [-26]8197.47

    $ stardate -n
    -298808.22

    $ stardate -g '[23]4906.5'
    2527-11-27T13:29:44

    $ stardate -s -n -g 2364-01-01
    [21]41000.15 41000.00 2364-01-01T00:00:00

## Two stardate systems

The tool supports two stardate systems that both use an epoch of
January 1, 2323 and produce roughly 1000 units per year, but define
the rate differently.

### Issue-based (`-s`) -- from the Stardates FAQ

Time is divided into *issues* shown in square brackets. In the TNG era
(issue 21+), each issue is exactly one quarter of the 400-year Gregorian
cycle (36524.25 days) containing 100,000 stardate units. This gives a
**constant** rate of one unit per ~31557 seconds (~8 h 46 min),
regardless of leap years.

Because a calendar year is 365 or 366 days and the rate is fixed,
stardates do not advance by exactly 1000 per year:

| Year type | Units/year | Drift from 1000 |
|-----------|-----------|-----------------|
| Normal (365 d) | 999.34 | -0.66 |
| Leap (366 d) | 1002.07 | +2.07 |

This drift accumulates. January 1, 2364 comes out as `[21]41000.15`
instead of a round `41000.00`, and by TNG's finale the shift approaches
one full unit.

### TNG-style (`-n`) -- matching on-screen stardates

Uses the formula:

    stardate = (year - 2323) * 1000 + (day_of_year / days_in_year) * 1000

This produces **exactly** 1000 units per calendar year, so year boundaries
always fall on round thousands. The trade-off is a slightly variable rate
(one unit = 31536 s in a normal year, 31622 s in a leap year).

This matches the convention the TNG writers actually used (documented in
the *TNG Technical Manual* and *Star Trek Chronology* by Okuda), where
the leading digit tracks the season number: season 1 = 41xxx,
season 2 = 42xxx, etc.

Canonical on-screen stardates round-trip exactly:

    $ stardate -s -n 41153.7
    [21]41154.17 41153.70

## Tests

    make test

## License

BSD 4-clause. See the license header in `stardate.c`.
