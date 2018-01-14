# CLBalancer

This repository is a basic mirror of `clbalancer`. Only the minimal runnable part of the project is clonned in this repository to avoid struggling with many not so relevant files.

**Work In Progress**.

## Hits

1. New API (+ maintanability). 20 vs 250 Maat lines.
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

## Run

Examples:

```sh
# assign static, device 0
./build/debug/clbalancer 0 0 0 1 1024 128 0.01 21
# vecadd dynamic, 2 devices
./build/debug/clbalancer 1 2 1 1 1024 128 0.10 21
# gaussian static, 2 devices
./build/debug/clbalancer 0 2 2 1 1024 128 0.11 21

# vecadd hguided, no checks, 2 devices
./build/release/clbalancer 2 2 1 0 100000 1000 0.5

# gaussian hguided, 2 devices
./build/release/clbalancer 2 2 2 1 2048 128 0.21 31

## gaussian static, 2 devices
/usr/bin/time ./build/debug/clbalancer 0 2 2 1 4096 524288 0.01 41
/usr/bin/time ./build/debug/clbalancer 0 2 2 1 1024 128 0.52 21
```

Concurrency tests:

```sh
scheds=(0 1) its=10 prop=0.01 timeout=15; for sched in ${scheds[@]}; do echo "scheduler $sched"; for i in $(seq 1 1 $its); do printf "$i "; out="$(timeout $timeout ./build/release/clbalancer $sched 2 2 1 2048 128 $prop 21)"; if [[ $? == 124 ]]; then printf "<-- Timeout "; else rt=$(echo "$out" | grep Success); [[ $? != 0 ]] && printf "<-- Failure "; fi; done; done
```

## Author

Raúl Nozal <raul.nozal@unican.es>

## License

Copyright 2017. Raúl Nozal <raul.nozal@unican.es>

To be specified.
