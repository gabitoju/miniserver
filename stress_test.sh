#!/bin/bash

if [ -z "$1" ]; then
    echo "Error: Target URL is required." >&2
    echo "Usage: $0 <target_url>" >&2
    exit 1
fi

TARGET="$1"
LOGFILE="stress_test_$(date +%Y%m%d_%H%M%S).log"

echo "==========================================" | tee -a "$LOGFILE"
echo "   STRESS TEST FOR $TARGET" | tee -a "$LOGFILE"
echo "   Started at: $(date)" | tee -a "$LOGFILE"
echo "==========================================" | tee -a "$LOGFILE"
echo "" | tee -a "$LOGFILE"

function section() {
    echo "" | tee -a "$LOGFILE"
    echo "-------------------------------------------------" | tee -a "$LOGFILE"
    echo "$1" | tee -a "$LOGFILE"
    echo "-------------------------------------------------" | tee -a "$LOGFILE"
}

# -------------------------------------------------
# 0) KEEP-ALIVE TESTS
# -------------------------------------------------

section "0) Keep-Alive test with curl (verbose)"

curl -v -H "Connection: keep-alive" "$TARGET" 2>&1 | tee -a "$LOGFILE"


section "0.1) Sequential Keep-Alive test (curl reusing connection)"

curl -v \
     -H "Connection: keep-alive" \
     "$TARGET" \
     --next "$TARGET" 2>&1 | tee -a "$LOGFILE"


section "0.2) ApacheBench Keep-Alive test (-k)"

ab -n 500 -c 1 -k "$TARGET" 2>&1 | tee -a "$LOGFILE"


section "0.3) Raw TCP Keep-Alive test (netcat)"

HOST=$(echo "$TARGET" | sed 's#http://##; s#/##')

{
  echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
  sleep 1
  echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
  sleep 1
} | nc $HOST 2>&1 | tee -a "$LOGFILE"


# -------------------------------------------------
# 1) BASELINE TEST
# -------------------------------------------------

section "1) Baseline: 500 sequential requests (-c 1)"

ab -n 500 -c 1 "$TARGET" 2>&1 | tee -a "$LOGFILE"


# -------------------------------------------------
# 2) QUEUEING BEHAVIOR
# -------------------------------------------------

section "2) Concurrency test: 2000 requests @ 20 concurrency"

ab -n 2000 -c 20 "$TARGET" 2>&1 | tee -a "$LOGFILE"


section "3) Concurrency test: 3000 requests @ 50 concurrency"

ab -n 3000 -c 50 "$TARGET" 2>&1 | tee -a "$LOGFILE"


# -------------------------------------------------
# 3) WRK LATENCY DISTRIBUTION
# -------------------------------------------------

section "4) WRK: 4 threads, 100 connections, 20 seconds"

wrk -t4 -c100 -d20s "$TARGET" 2>&1 | tee -a "$LOGFILE"


# -------------------------------------------------
# 4) EXTREME LOAD TEST
# -------------------------------------------------

section "5) WRK: Extreme test – 8 threads, 400 connections, 30 seconds"

wrk -t8 -c400 -d30s "$TARGET" 2>&1 | tee -a "$LOGFILE"


# -------------------------------------------------
# SUMMARY
# -------------------------------------------------

section "Keep-Alive RESULT SUMMARY"

if grep -q "Re-using existing connection" "$LOGFILE"; then
    echo "OK: Keep-Alive verified (curl connection reused)." | tee -a "$LOGFILE"
else
    echo "WARNING: Keep-Alive may NOT be working (curl did not reuse connection)." | tee -a "$LOGFILE"
fi

if grep -q "left intact" "$LOGFILE"; then
    echo "OK: Server kept connection open as expected." | tee -a "$LOGFILE"
fi

if grep -q "Closing connection" "$LOGFILE"; then
    echo "Server is closing connections; check Keep-Alive logic." | tee -a "$LOGFILE"
fi


section "FINISHED"
echo "Stress test completed at: $(date)" | tee -a "$LOGFILE"
echo "Full log saved to: $LOGFILE" | tee -a "$LOGFILE"
