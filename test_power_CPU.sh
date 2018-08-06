
#watermarking
#static
#path=test_watermark/long_exe
#
#num=6
#
#base=all
##base=all measurement
##allt= only alltilization without NDRange and Read
#
#for dev in 7 
#do
#
#perf stat -r 1 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 0 $dev 6 0 19537200 62499968 1 0.5 21
#
#done

##for dev in  4 
##do
##
##perf stat -r 10 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 $dev 6 0 19537200 21874944 1 0.5 21
##
##done
##
###dynamic
#perf stat -r 1 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 7 6 0 19537200 4194304 1 0.5 21
##Hguided
#perf stat -r 1 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 2 7 6 0 19537200 1024 0.5 21 
#
#
#path=test_sobel/long_exe
#
#num=5
#size=312595200
#chunk_size=1048576
##base=all
##
###base=all measurement
###allt= only alltilization without NDRange and Read
#
#for dev in 7 
#do
#
#perf stat -r 10 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 0 $dev $num 0 $size 1000000000 1 0.5 21
#
#done
#
##for dev in 4
##do
##
##perf stat -r 10 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 $dev $num 0 $size 350000000 1 0.5 21
##
##done
##
##
###dynamic
#perf stat -r 1 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 7 $num 0 $size $chunk_size 1 1.5 21
###Hguided
#perf stat -r 1 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 2 7 $num 0 $size 1024 1 0.5 21
##
##
#path=test_mersenne/long_exe
#
#num=4
##size=7812500
#size=5500000
#chunk_size=`expr $size/30`
#base=all
#
##base=all measurement
##allt= only alltilization without NDRange and Read
#
#for dev in 7
#do
#
#perf stat -r 1 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 0 $dev $num 0 $size 1953792 1 0.5 21
#
#sleep 10
#done
###dynamic
#perf stat -r 10 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 7 $num 0 $size $chunk_size 1 0.5 21
##Hguided
#sleep 10
#perf stat -r 10 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 2 7 $num 0 $size 1953792 1 0.5 21
#
#
#path=test_matrixmult
#
#num=7
#size=255
#chunk_size=128
#base=all
#
#base=all measurement
#allt= only alltilization without NDRange and Read
#
#for dev in 1 2 4 7
#do
#
#perf stat -r 10 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 0 $dev $num 0 $size 1953792 1 0.5 21
#
#done
#
##dynamic
#perf stat -r 10 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 7 $num 0 $size $chunk_size 1 0.5 21
##Hguided
#perf stat -r 10 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 2 7 7 0 $size 1 1 0.5 0.4
#
#
path=test_nn/long_exe

num=11
size=11800
chunk_size=1000000000
base=all

for dev in 7
do

perf stat -r 1 -x - -o $path/epowercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 0 $dev $num 0 $size 1953792 1 0.5 21

sleep 5
done

#
#chunk_size=350000000
#perf stat -r 1 -x - -o $path/powercpu_all_static_4.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 4 $num 0 $size 1953792 1 0.5 21
#
#sleep 5
#
#
#chunk_size=4194304
##dynamic
#perf stat -r 1 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 7 $num 0 $size $chunk_size 1 0.5 21
###Hguided
#sleep 5
#perf stat -r 1 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 2 7 $num 0 $size 1024 1 0.5 21
#
path=test_aes/long_exe

num=8
size=255
chunk_size=8388608
base=all
 #
 #for dev in 1 2 
 #do
 #
 #perf stat -r 10 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 $dev $num 0 $size 87500000 1 0.5 21
 #
 #done
#for dev in  7 
#do
#
#perf stat -r 1 -x - -o $path/powercpu_all_static_$dev.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 0 $dev $num 0 $size 87500000 1 0.5 21
#
#done
#
#dynamic
#perf stat -r 1 -x - -o $path/powercpu_all_dynamic_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 1 7 $num 0 $size $chunk_size 1 0.5 21
#Hguided
#perf stat -r 2 -x - -o $path/powercpu_all_hguided_7.txt -a -e power/energy-pkg/,power/energy-cores/,power/energy-ram/,power/energy-gpu/,cycles ./build/debug/EngineCL 2 7 $num 0 $size 1024 1 0.5 21


#
#
