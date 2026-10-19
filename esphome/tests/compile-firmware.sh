#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT HUP INT TERM
cp "$root/neopixel-clock-esp32.yaml" "$root/neopixel-clock-c3.yaml" "$stage/"
cp "$root/secrets.yaml.example" "$stage/secrets.yaml"
cp -R "$root/packages" "$stage/packages"
ln -s "$root/components" "$stage/components"

for board in esp32 c3; do
  # Keep generated firmware in an ignored cache, not in the disposable config tree.
  printf '\nesphome:\n  build_path: "%s/.esphome/build/neopixel-clock-%s"\n' "$root" "$board" \
    >> "$stage/neopixel-clock-$board.yaml"
  "$root/.venv/bin/esphome" compile "$stage/neopixel-clock-$board.yaml"
done
