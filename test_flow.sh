max=10
#rm data3BS_D.txt
#CU= Callback with reading un blocking
#offset support on, not use in -NDRange
#name_file=data_twist_CPU.txt
#rm $name_file
#for i in `seq 1 $max` 
#do
# let dim=819200*i
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
name_file=data_twist_FPGA.txt
 rm $name_file
 for i in `seq 1 $max` 
 do
  let dim=819200*i
  echo $dim
  
   ./build/debug/clbalancer 0 4 4 0 $dim 128 0.52 21 &>>$name_file 
 
  (tail -n2) < $name_file
   echo "$dim" &>> $name_file
   echo '/build/debug/clbalancer 0 4 4 0 $dim 128 0.52 21' &>>$name_file

   sleep 5
   echo $i
done

name_file=data_twist_GPU.txt
 rm $name_file
 for i in `seq 1 $max` 
 do
  let dim=819200*i
  echo $dim
  
   ./build/debug/clbalancer 0 2 4 0 $dim 128 0.52 21 &>>$name_file 
 
  (tail -n2) < $name_file
   echo "$dim" &>> $name_file
   echo '/build/debug/clbalancer 0 2 4 0 $dim 128 0.52 21' &>>$name_file

   sleep 5
   echo $i
done
