#mandelbrot
#input argument 1: Device
#./build/debug/EngineCL 1 $1 5 0 1024 62519040 0.52 21
if [ $# -eq 0 ]
  then
    echo -e "No arguments supplied, needs to select the devices:\n 1-CPU\n 2-GPU\n 4-FPGA"
    exit 1
fi

./build/debug/EngineCL 2 $1 5 0 1024 312595200 0.52 21


