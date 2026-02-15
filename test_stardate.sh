#!/bin/bash
# Test suite for stardate program
# Compares output against known-good reference values

STARDATE=./stardate
PASS=0
FAIL=0

check() {
  local desc="$1"
  shift
  local expected="$1"
  shift
  local actual
  actual=$("$STARDATE" "$@" 2>&1)
  if [ "$actual" = "$expected" ]; then
    PASS=$((PASS + 1))
  else
    FAIL=$((FAIL + 1))
    echo "FAIL: $desc"
    echo "  args:     $*"
    echo "  expected: $expected"
    echo "  actual:   $actual"
  fi
}

# TNG stardate to Gregorian
check "TNG stardate to Gregorian" \
  "2527-11-27T13:29:44" \
  -g '[23]4906.5'

# TNG stardate all formats
check "TNG stardate all formats" \
  "[23]04906.50 2527=11=10T13:29:44 2527-11-27T13:29:44 2527*11*27T20:56:24 U17605776584 U0x41962d4c8" \
  -s -j -g -q -u -x '[23]4906.5'

# Gregorian to stardate
check "Gregorian 2024-01-15 to stardate" \
  "[-26]8035.00" \
  -s 2024-01-15

# Gregorian to all formats
check "Gregorian 2024-01-15 all formats" \
  "[-26]8035.00 2024=01=02T00:00:00 2024-01-15T00:00:00 2024*01*15T11:56:55 U1705276800 U0x65a47580" \
  -s -j -g -q -u -x 2024-01-15

# Gregorian to Unix time
check "Gregorian to Unix time" \
  "U1705276800" \
  -u 2024-01-15

# Unix epoch to stardate
check "Unix epoch to stardate" \
  "[-36]9350.00" \
  -s U0

# Unix epoch to all formats
check "Unix epoch all formats" \
  "[-36]9350.00 1969=12=19T00:00:00 1970-01-01T00:00:00 1970*01*01T14:27:01 U0 U0x0" \
  -s -j -g -q -u -x U0

# Negative stardate to Gregorian
check "Negative stardate to Gregorian" \
  "1997-12-26T19:00:28" \
  -g '[-30]0458.96'

# Negative stardate all formats
check "Negative stardate all formats" \
  "[-30]0458.96 1997=12=13T19:00:28 1997-12-26T19:00:28 1997*12*27T14:34:40 U883162828 U0x34a3fecc" \
  -s -j -g -q -u -x '[-30]0458.96'

# TOS era stardate
check "TOS era [0]1000" \
  "[0]1000.00 2162=07=09T00:00:00 2162-07-23T00:00:00 2162*07*23T21:46:07 U6076512000 U0x16a303700" \
  -s -j -g -q -u -x '[0]1000'

# Film era boundary [19]7340
check "Film era boundary [19]7340" \
  "[19]7340.00 2270=01=11T00:00:00 2270-01-26T00:00:00 2270*01*26T20:02:52 U9469267200 U0x234698d00" \
  -s -j -g -q -u -x '[19]7340'

# Film era [19]7840
check "Film era [19]7840" \
  "[19]7840.00 2283=09=20T00:00:00 2283-10-05T00:00:00 2283*10*05T12:22:29 U9901267200 U0x24e295900" \
  -s -j -g -q -u -x '[19]7840'

# Late film era [20]5005
check "Late film era [20]5005" \
  "[20]5005.00 2322=12=14T00:00:00 2322-12-30T00:00:00 2322*12*30T00:01:54 U11139379200 U0x297f57000" \
  -s -j -g -q -u -x '[20]5005'

# TNG epoch [21]00000
check "TNG epoch [21]00000" \
  "[21]00000.00 2322=12=16T00:00:00 2323-01-01T00:00:00 2323*01*01T00:00:00 U11139552000 U0x297f81300" \
  -s -j -g -q -u -x '[21]00000'

# Negative stardate issue [-1]
check "Negative issue [-1]0000" \
  "[-1]0000.00 2156=06=30T00:00:00 2156-07-14T00:00:00 2156*07*15T08:49:29 U5886432000 U0x15edbd300" \
  -s -j -g -q -u -x '[-1]0000'

# Julian epoch
check "Julian epoch 0001=01=01" \
  "[-395]3530.00 0001=01=01T00:00:00 0000-12-30T00:00:00 0000*12*31T02:03:16 U-62135769600 U-0xe77949a00" \
  -s -j -g -q -u -x '0001=01=01'

# Gregorian epoch
check "Gregorian epoch 0001-01-01" \
  "[-395]3540.00 0001=01=03T00:00:00 0001-01-01T00:00:00 0001*01*02T02:01:21 U-62135596800 U-0xe7791f700" \
  -s -j -g -q -u -x 0001-01-01

# 1970-01-01 via Gregorian
check "1970-01-01 Gregorian" \
  "[-36]9350.00 1969=12=19T00:00:00 1970-01-01T00:00:00 1970*01*01T14:27:01 U0 U0x0" \
  -s -j -g -q -u -x 1970-01-01

# Y2K
check "Y2K 2000-01-01" \
  "[-30]4135.00 1999=12=19T00:00:00 2000-01-01T00:00:00 2000*01*01T07:51:17 U946684800 U0x386d4380" \
  -s -j -g -q -u -x 2000-01-01T00:00:00

# Quadcent epoch
check "Quadcent epoch 0323*01*01" \
  "[-336]1575.00 0322=12=31T00:00:00 0323-01-01T00:00:00 0323*01*01T00:00:00 U-51974352000 U-0xc19e9ac80" \
  -s -j -g -q -u -x '0323*01*01'

# Unix time 1 billion
check "Unix time 1 billion" \
  "[-30]7220.37 2001=08=27T01:46:40 2001-09-09T01:46:40 2001*09*09T23:47:58 U1000000000 U0x3b9aca00" \
  -s -j -g -q -u -x U1000000000

# Unix time hex input
check "Unix time hex 0x3B9ACA00" \
  "[-30]7220.37 2001=08=27T01:46:40 2001-09-09T01:46:40 2001*09*09T23:47:58 U1000000000 U0x3b9aca00" \
  -s -j -g -q -u -x U0x3B9ACA00

# Negative Unix time
check "Negative Unix time U-86400" \
  "[-36]9345.00 1969=12=18T00:00:00 1969-12-31T00:00:00 1969*12*31T14:27:58 U-86400 U-0x15180" \
  -s -j -g -q -u -x U-86400

# Stardate precision s0
check "Stardate precision s0" \
  "[-30]0458" \
  -s0 '[-30]0458.96'

# Stardate precision s6
check "Stardate precision s6" \
  "[-30]0458.960000" \
  -s6 '[-30]0458.96'

# Round-trip: stardate -> gregorian -> stardate (loses sub-second precision)
check "Round-trip TNG via Gregorian" \
  "[23]04906.49" \
  -s 2527-11-27T13:29:44

# Round-trip: unix -> stardate -> unix
check "Round-trip Unix 1B via stardate" \
  "[-30]7220.37" \
  -s U1000000000

# -v prints version
check "Version flag" \
  "stardate 1.6.2" \
  -v

# -h starts with Usage:
actual=$("$STARDATE" -h 2>&1)
if echo "$actual" | head -1 | grep -q '^Usage:'; then
  PASS=$((PASS + 1))
else
  FAIL=$((FAIL + 1))
  echo "FAIL: Help flag starts with Usage:"
  echo "  actual: $actual"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed out of $((PASS + FAIL)) tests"
if [ "$FAIL" -ne 0 ]; then
  exit 1
fi
