#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root="${1:-outputs/reproduction}"
for seed in 42 99; do
 build/cobweb_lab --preset radial --out "$root/radial_$seed" --atlas 2048 --density .5 --seed "$seed" --broken .03
 build/cobweb_lab --preset corner --out "$root/corner_$seed" --atlas 2048 --density .65 --seed "$seed" --broken .25
 build/cobweb_lab --preset corridor --out "$root/corridor_$seed" --atlas 4096 --density .9 --seed "$seed" --broken .18 --layers 2 --width .0045
done
