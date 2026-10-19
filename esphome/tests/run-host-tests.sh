#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
esphome_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM

cxx=${CXX:-c++}
"$cxx" \
  -std=c++11 \
  -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$esphome_dir" \
  "$script_dir/test_core.cpp" \
  "$esphome_dir/components/modular_7segment/segment_renderer.cpp" \
  "$esphome_dir/components/modular_7segment/display_policy.cpp" \
  -o "$build_dir/test_core"
"$build_dir/test_core"
