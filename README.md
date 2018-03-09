# EngineCL

This repository is a basic mirror of `EngineCL`. Only the minimal runnable part of the project is clonned in this repository to avoid struggling with many not so relevant files.

**Work In Progress**.

**Disclaimer**: This library is private until the author releases it.

## Hits

1. New API (+ maintanability). 33 simple lines vs 105 more complex Maat lines (eg. in gaussian).
1. New Architecture (robust and flexible).
1. Pure Library (can be reused). Maat it is not.
1. Achieved LBA: Static, Dynamic and HGuided.

## Build

```sh
make build/debug
make build/release

# legacy:
OPENCL_VERSION=1.2 make build/release
```

## Tools: clkernel

To build binary kernels and be able to use them directly with the new feature of Devices (custom kernel).

```sh
make build/debug-clkernel

# discovery:
./build/debug/tools/clkernel

# build:
./build/debug/tools/clkernel 0 0 support/kernels/vecadd_offset.cl
# Writting to support/kernels/vecadd_offset_station:0:0.cl.bin
HOSTNAME=xxx ./build/debug/tools/clkernel 0 1 support/kernels/vecadd_offset.cl
# Writting to support/kernels/vecadd_offset_xxx:0:1.cl.bin
```

## Run

Examples:

```sh
# assign static, device 0
./build/debug/EngineCL 0 0 0 1 1024 128 0.01 21
# vecadd dynamic, 2 devices
./build/debug/EngineCL 1 2 1 1 1024 128 0.10 21
# gaussian static, 2 devices
./build/debug/EngineCL 0 2 2 1 1024 128 0.11 21

# vecadd hguided, no checks, 2 devices
./build/release/EngineCL 2 2 1 0 100000 1000 0.5

# gaussian hguided, 2 devices
./build/release/EngineCL 2 2 2 1 2048 128 0.21 31

## gaussian static, 2 devices
/usr/bin/time ./build/debug/EngineCL 0 2 2 1 4096 524288 0.01 41
/usr/bin/time ./build/debug/EngineCL 0 2 2 1 1024 128 0.52 21
```

Concurrency tests:

```sh
scheds=(0 1) its=10 prop=0.01 timeout=15; for sched in ${scheds[@]}; do echo "scheduler $sched"; for i in $(seq 1 1 $its); do printf "$i "; out="$(timeout $timeout ./build/release/EngineCL $sched 2 2 1 2048 128 $prop 21)"; if [[ $? == 124 ]]; then printf "<-- Timeout "; else rt=$(echo "$out" | grep Success); [[ $? != 0 ]] && printf "<-- Failure " && echo "$out"; fi; done; done
```

## Author

Raúl Nozal <raul.nozal@unican.es>

## License

MIT License.

Copyright (c) 2017. Raúl Nozal <raul.nozal@unican.es>

See LICENSE file.
