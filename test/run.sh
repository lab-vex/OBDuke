#!/bin/bash
# Compiles the real firmware on a PC against a simulated Duke (engine ECU, ABS, broadcast
# traffic) and checks ISO-TP, codes, modules, sniffer, signals, gear and the ELM327 / app links.
set -e
cd "$(dirname "$0")"
for t in test_core test_bluetooth; do
  g++ -std=gnu++17 -g -fsanitize=address,undefined -Imock -Imock/driver -o /tmp/$t $t.cpp
  echo "== $t"
  /tmp/$t > /tmp/$t.log 2>&1 || true
  grep -E "FAIL|failure|runtime error" /tmp/$t.log
  grep -q "^0 failure(s)" /tmp/$t.log || { cat /tmp/$t.log; exit 1; }
done
echo "All tests passed"
