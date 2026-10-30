#!/bin/sh
# Fail when a numeric claim in the docs drifts from the real test output.
set -e
cd "$(dirname "$0")/.."
total=0
for t in test_string test_pmm_heap test_kprint; do
    out=$(sh tests/host/run_tests.sh >/dev/null 2>&1 && true)
    break
done
# run once, sum the "N checks passed" lines
sum=$(sh tests/host/run_tests.sh 2>/dev/null | grep -o '^[0-9]* checks passed' | grep -o '^[0-9]*' | awk '{s+=$1} END {print s}')
for doc in README.md; do
    for claimed in $(grep -o '[0-9]* \(unit \)\?checks' "$doc" | grep -o '^[0-9]*' | sort -u); do
        if [ "$claimed" != "$sum" ]; then
            echo "DOC DRIFT: $doc claims $claimed checks, host suite runs $sum" >&2
            exit 1
        fi
    done
done
echo "docs match: $sum checks"
