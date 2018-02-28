max=20
#rm data3BS_D.txt
#CU= Callback with reading un blocking
#offset support on, not use in -NDRange
#name_file=data_twist_CPU.txt
#rm $name_file
#for i in `seq 1 $max` 
#do
# let dim=1024*i
# echo $dim
# 
#  ./build/debug/clbalancer 0 1 4 0 $dim 128 0.52 21 &>>$name_file 
#
# (tail -n2) < $name_file
#  echo "$dim" &>> $name_file
#  echo '/build/debug/clbalancer 0 1 4 0 $dim 128 0.52 21' &>>$name_file
##  ./build/debug/clbalancer 1 3 2 1 1024 128 0.52 21 &>> data3B_D.txt
##  (tail -n1) <data3B_D.txt
## DIFF= $(diff gauss.txt master_test.txt)
#
#  sleep 5 
#  echo $i 
#done
#
#name_file=data_twist_FPGA.txt
# rm $name_file
# for i in `seq 1 $max` 
# do
#  let dim=1024*i
#  echo $dim
#  
#   ./build/debug/clbalancer 0 4 4 0 $dim 128 0.52 21 &>>$name_file 
# 
#  (tail -n2) < $name_file
#   echo "$dim" &>> $name_file
#   echo '/build/debug/clbalancer 0 4 4 0 $dim 128 0.52 21' &>>$name_file
#
#   sleep 5
#   echo $i
#done

name_file=chunk_size_impact.txt
name_file2=chunk_size_impact_time.txt
 rm $name_file
for dev in 4 2 1
do
  for sample in `seq 1 $max`
  do
     for i in 2073600 691200 207360 103680 41472 20736 10368
     do
  
      ./build/debug/clbalancer 1 $dev 5 0 1024 $i 0.52 21 &>>$name_file 
 
      (tail -n2) < $name_file
      (tail -n2) < $name_file>>$name_file2
  
        echo '/build/debug/clbalancer 1 $dev 5 0 $dim 128 0.52 21' &>>$name_file

      sleep 5
      echo $i
    done
  done
 done
grep -v "Done Sobel" $name_file2 > file2.txt; mv file2.txt $name_file2
 #name_file=data_twistDy_FCG.txt
 # rm $name_file
 # for i in `seq 1 $max` 
 # do
 #  let dim=2048*3*i*i
 #  echo $dim
 #  
 #   ./build/debug/clbalancer 1 7 4 0 $dim 128 0.52 21 &>>$name_file 
 # 
 #  (tail -n2) < $name_file
 #   echo "$dim" &>> $name_file
 #   echo '/build/debug/clbalancer 1 7 4 0 $dim 128 0.52 21' &>>$name_file
 #
 #   sleep 5
 #   echo $i
 #done
