#query to measume the power in a kernel, test for execution file


for Nt in 1 2 3 4 5 6 7 8 9 

do


#watermarking
#static
path=test_watermark/long_exec
num=6
base=all
for dev in 2 7 
  do
    nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_static_$dev.txt &
    pid_c=$!
    echo $pid_c
   ./build/debug/clbalancer 0 $dev 6 0 19537200 1953792 1 0.5 21
    kill -9 $pid_c
  sleep 60 
done

#dynamic
  nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_dynamic_7.txt &
    pid_c=$!
    echo $pid_c
   ./build/debug/clbalancer 1 7 6 0 19537200 1953792 1 0.5 21
    kill -9 $pid_c
#Hguided
  sleep 60 

 nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_hguided_7.txt &
    pid_c=$!
    echo $pid_c
    ./build/debug/clbalancer 2 7 6 0 19537200 1953792 1 0.5 21
    kill -9 $pid_c
  sleep 60 
#path=test_sobel
#
#num=5
#size=312595200
#chunk_size=31259520
#base=all
#
##base=all measurement
##allt= only alltilization without NDRange and Read
#
#for dev in 2 7
#do
#    nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_static_$dev.txt &
#    pid_c=$!
#    echo $pid_c
# 
#  ./build/debug/clbalancer 0 $dev $num 0 $size 1953792 1 0.5 21
#  sleep 4 
#    kill -9 $pid_c
#
#  sleep 60 
#done
#
##dynamic
#
#  nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_dynamic_7.txt &
#    pid_c=$!
#    echo $pid_c
#
#./build/debug/clbalancer 1 7 $num 0 $size $chunk_size 1 0.5 21
#  sleep 4 
#    kill -9 $pid_c
#
##Hguided
#  sleep 60 
#
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_hguided_7.txt &
#    pid_c=$!
#    echo $pid_c
#  ./build/debug/clbalancer 2 7 $num 0 $size 1953792 1 0.5 21
#  sleep 4 
#    kill -9 $pid_c
#
#
#  sleep 60 
##path=test_mersenne
##
##num=4
##size=7812500
##chunk_size=`expr $size/30`
##base=all
##
###base=all measurement
###allt= only alltilization without NDRange and Read
###
###for dev in 1 2 4 7
###do
###
###perf stat -r 10 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/clbalancer 0 $dev $num 0 $size 1953792 1 0.5 21
###
###done
###
###dynamic
###perf stat -r 10 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/clbalancer 1 7 $num 0 $size $chunk_size 1 0.5 21
####Hguided
###perf stat -r 10 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/clbalancer 2 7 $num 0 $size 1953792 1 0.5 21
##
##
#path=test_matrixmult
#
#num=7
#size=255
#chunk_size=128
#
#for dev in 2 7
#do
#
#    nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_static_$dev.txt &
#    pid_c=$!
#    echo $pid_c
#    ./build/debug/clbalancer 0 $dev $num 0 $size 1953792 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
#  sleep 60 
#done
#
##dynamic
#
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_dynamic_7.txt &
#    pid_c=$!
#    echo $pid_c
# ./build/debug/clbalancer 1 7 $num 0 $size $chunk_size 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
#  sleep 60 
#
##Hguided
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_hguided_7.txt &
#    pid_c=$!
#    echo $pid_c
# ./build/debug/clbalancer 2 7 7 0 $size 1 1 0.5 0.4
#  sleep 4 
#    kill -9 $pid_c
#  sleep 60 
#
#
#path=test_nn
#
#num=11
#size=85000
#chunk_size=8388608
#base=all
#
#for dev in 2 7
#do
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_static_$dev.txt &
#    pid_c=$!
#    echo $pid_c
#
# ./build/debug/clbalancer 0 $dev $num 0 $size 1953792 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
#  sleep 60 
#done
#
##dynamic
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_dynamic_7.txt &
#    pid_c=$!
#    echo $pid_c
#
# ./build/debug/clbalancer 1 7 $num 0 $size $chunk_size 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
#
#
##Hguided
#  sleep 60 
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_hguided_7.txt &
#    pid_c=$!
#    echo $pid_c
# ./build/debug/clbalancer 2 7 $num 0 $size 1953792 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
#  sleep 60 
#
#path=test_aes
#
#num=8
#size=255
#chunk_size=976850
#base=all
#
#for dev in 2 7
#do
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_static_$dev.txt &
#    pid_c=$!
#    echo $pid_c
# ./build/debug/clbalancer 0 $dev $num 0 $size 1953792 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
#  sleep 60 
#done
#
##dynamic
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_dynamic_7.txt &
#    pid_c=$!
#    echo $pid_c
#
# ./build/debug/clbalancer 1 7 $num 0 $size $chunk_size 1 0.5 21
#sleep 4 
#    kill -9 $pid_c
#
##Hguided
#  sleep 60 
# nvidia-smi --query-gpu=pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used,name,timestamp --format=csv -l 1 > $path/powergpu_all"$Nt"_hguided_7.txt &
#    pid_c=$!
#    echo $pid_c
# ./build/debug/clbalancer 2 7 $num 0 $size 1953792 1 0.5 21
#
#  sleep 4 
#  kill -9 $pid_c
#
#
#  sleep 60 
#
#timeout 10000 nvidia-smi --query-gpu=name,pstate,power.draw,temperature.gpu,utilization.gpu,utilization.memory,memory.used --format=csv -l 1 > results-file.csv
done
