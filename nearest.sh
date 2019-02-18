if [ $# -eq 0 ]
  then
    echo -e "No arguments supplied, needs to select the devices:\n 1-CPU\n 2-GPU\n 4-FPGA"
    exit 1
fi

./build/debug/EngineCL 1 $1 11 0 11800 40148000 0.3 21

