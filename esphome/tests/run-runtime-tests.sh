#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary=$(mktemp)
trap 'rm -f -- "$binary"' EXIT HUP INT TERM
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -Wno-unused-parameter \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$root/tests/fakes" -I"$root/components/modular_7segment" \
  "$root/tests/test_runtime.cpp" \
  "$root/components/modular_7segment/modular_7segment.cpp" \
  "$root/components/modular_7segment/segment_renderer.cpp" \
  "$root/components/modular_7segment/display_policy.cpp" \
  -o "$binary"
"$binary"
