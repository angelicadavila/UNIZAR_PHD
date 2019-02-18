if [ $# -eq 0 ]
  then
    echo -e "No arguments supplied, needs to select the devices:\n 1-CPU\n 2-GPU\n 4-FPGA"
    exit 1
fi


let _size=`expr 5500000`
#let chunk_size=`expr 4200000`
let chunk_size=`expr 8386688`

./build/debug/EngineCL 1 $1 4 0 $_size $chunk_size 0.1335 21
