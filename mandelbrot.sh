if [ $# -eq 0 ]
  then
    echo -e "No arguments supplied, needs to select the devices:\n 1-CPU\n 2-GPU\n 4-FPGA"
    exit 1
fi

#mandelbrot
#input argument 1: Device
#./build/debug/EngineCL 1 $1 12 0 1024 1024 0.52 21
./build/debug/EngineCL 1 $1 12 0 1024 20480 0.52 21


