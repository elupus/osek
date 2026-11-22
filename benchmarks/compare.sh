#!/usr/bin/env bash
# Builds the OSEK metric apps (riscv32-qemu preset) and their counterparts
# for every other RTOS in benchmarks/, runs each under QEMU with -icount so
# that one second of virtual time is exactly 10^9 instructions, and prints
# one table of the counts. See benchmarks/README.md.
#
# Every directory benchmarks/<os>/ with a CMakeLists.txt is a system to
# compare: a standalone CMake project that builds an app
# <Prefix>_Metric<Name>/<Prefix>_Metric<Name> for each metric it implements.
# The column is named after <Prefix>, taken from the app directories.
#
# Needs riscv64-unknown-elf-gcc, picolibc, qemu-system-riscv32, cmake and
# ninja, e.g. on Debian/Ubuntu:
#   gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf cmake ninja-build
#   qemu-system-riscv (qemu-system-misc on older releases)

set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
osek_build=$root/build/riscv32-qemu

# The metrics, in table order, as named by the OSEK apps test/Os_Metric<Name>
metrics=(Basic Cooperative Preemptive Events)

# Operations per reported count: each of the five cooperative tasks counts
# one switch, the others count one operation per round.
ops_per_count() {
    case $1 in
        Cooperative) echo 5 ;;
        *)           echo 1 ;;
    esac
}

# Systems to compare: OSEK first, then benchmarks/<os> in name order.
# names[i] is the column title, and srcs[i] and apps[i] are the source
# directory and the ELF file of an app, with @ in place of the metric name.
names=(OSEK)
srcs=("$root/test/Os_Metric@")
apps=("$osek_build/test/Os_Metric@/Os_Metric@")

echo "== Building OSEK (riscv32-qemu)" >&2
osek_targets=()
for m in "${metrics[@]}"; do
    osek_targets+=("Os_Metric$m")
done
(cd "$root" && cmake --preset riscv32-qemu >/dev/null)
cmake --build "$osek_build" --target "${osek_targets[@]}" >/dev/null

for list in "$root"/benchmarks/*/CMakeLists.txt; do
    src=$(dirname "$list")
    os=$(basename "$src")
    app=$(cd "$src" && ls -d -- *_Metric*/ 2>/dev/null | head -n 1)
    if [ -z "$app" ]; then
        echo "error: no <Prefix>_Metric<Name> app in $src" >&2
        exit 1
    fi
    prefix=${app%%_Metric*}
    build=$root/build/bench-$os

    echo "== Building $prefix (benchmarks/$os)" >&2
    cmake -S "$src" -B "$build" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$root/toolchain/Toolchain-riscv32-elf.cmake" >/dev/null
    cmake --build "$build" >/dev/null

    names+=("$prefix")
    srcs+=("$src/${prefix}_Metric@")
    apps+=("$build/${prefix}_Metric@/${prefix}_Metric@")
done

# Prints the first number after "Execution counts" in the app's output.
# Semihosting output may arrive on QEMU's stderr, so capture both. Before
# main the app reads its command line, the -kernel argument, through
# semihosting, so it is run from its directory: the start up, and with it
# the counts, are then the same wherever the repository is checked out.
run() {
    local out
    out=$(cd "$(dirname "$1")" && timeout 120 qemu-system-riscv32 -M virt \
        -bios none -nographic -monitor none \
        -semihosting-config enable=on,target=native \
        -icount shift=0,sleep=off -kernel "$(basename "$1")" </dev/null 2>&1)
    local count
    count=$(sed -n 's/.*Execution counts \([0-9]*\).*/\1/p' <<<"$out")
    if [ -z "$count" ]; then
        echo "error: no count from $1: $out" >&2
        exit 1
    fi
    echo "$count"
}

per_op() {
    awk -v c="$1" -v n="$2" 'BEGIN { printf "%.0f", 1e9 / (c * n) }'
}

# One row of cells, padded to the width of the header cells.
row() {
    local cells=("$@") i
    local line="| $(printf '%-12s' "${cells[0]}") |"
    for ((i = 1; i < ${#cells[@]}; ++i)); do
        line+=" $(printf "%${widths[i]}s" "${cells[i]}") |"
    done
    echo "$line"
}

echo "== Running under QEMU with -icount shift=0,sleep=off" >&2
echo

header=(Metric)
for n in "${names[@]}"; do
    header+=("$n")
done
for n in "${names[@]}"; do
    header+=("$n ins/op")
done
widths=(12)
for ((i = 1; i < ${#header[@]}; ++i)); do
    w=${#header[i]}
    widths+=($((w < 9 ? 9 : w)))
done

row "${header[@]}"
line="|--------------|"
for ((i = 1; i < ${#header[@]}; ++i)); do
    line+="$(printf '%*s' $((widths[i] + 1)) '' | tr ' ' -):|"
done
echo "$line"

# A metric that a system does not implement shows as "-".
for m in "${metrics[@]}"; do
    n=$(ops_per_count "$m")
    counts=()
    ops=()
    for ((i = 0; i < ${#apps[@]}; ++i)); do
        if [ -d "${srcs[i]//@/$m}" ]; then
            c=$(run "${apps[i]//@/$m}")
            counts+=("$c")
            ops+=("$(per_op "$c" "$n")")
        else
            counts+=(-)
            ops+=(-)
        fi
    done
    row "$m" "${counts[@]}" "${ops[@]}"
done
