if [ $# -eq 0 ]
  then
    echo -e "No arguments supplied, needs to select the devices:\n 1-CPU\n 2-GPU\n 4-FPGA"
    exit 1
fi

  ./build/debug/EngineCL 1 $1 6 0 19537200 5861160 1 0.52 21
  #./build/debug/EngineCL 1 $1 6 0 19537200 9768570 1 0.52 21
