
#for dev in 4 2 1
#do
#  for sample in {1..20}
#  do
##    echo $dev
#    ./build/debug/clbalancer 0 $dev 6 0 1024 207360 0.52 21 &>>$name_file
#    (tail -n2) < $name_file
#    (tail -n2) < $name_file>>$name_file2
#    echo '/build/debug/clbalancer 0 $dev 6 0 1024 207360 0.52 21' &>>$name_file
#    sleep 5
#    echo $sample
#    
#  done
#done 
#
#grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2
#
#name_file=test_watermark/model_dynamic.txt
#  name_file2=test_watermark/modelt_dynamic.txt

#for dev in 4 2 1 7 6 5 3
#do
# #evaluating chunk size in performance of dynamic
#  for chunk in {1..500..5}
#   #3907430 1953715 1302476 976857 781486 651238 55820434
#   #312595200 3125952 625190 312595 156297 104198 
#  # 4096 40960 204800 409600 4096000
# #  312595200 3125952 625190 312595 156297 104198
#    do
#  for sample in {1..10}
#  do
#  let chunk_size=`expr 19537200/$chunk`
#  echo $chunk_size
#    echo $dev
#    ./build/debug/clbalancer 1 $dev 6 0 1024 $chunk_size 0.52 21 &>>$name_file
#    (tail -n2) < $name_file
#    (tail -n2) < $name_file>>$name_file2
#    echo '/build/debug/clbalancer 1 $dev 6 0 1024 $chunk_size 0.52 21' &>>$name_file
#    sleep 5
#    echo $sample
#  done
#  done
#done 
#
#grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2
#

name_file=test_watermark/model_dynamic.txt
  name_file2=test_watermark/modelt_dynamic.txt

for dev in 4 2 1 7 6 5 3
do
 #evaluating chunk size in performance of dynamic
   for chunk in {1..2000..50}
   #3907430 1953715 1302476 976857 781486 651238 55820434
   #312595200 3125952 625190 312595 156297 104198 
  # 4096 40960 204800 409600 4096000
 #  312595200 3125952 625190 312595 156297 104198
    do
  for sample in {1..5}
  do
#  let chunk_size=`expr 312595200/$chunk`
  let chunk_size=`expr 19537200/$chunk`
  echo $chunk_size
    echo $dev
    ./build/debug/clbalancer 1 $dev 6 0 1024 $chunk_size 0.52 21 &>>$name_file
    (tail -n2) < $name_file
    (tail -n2) < $name_file>>$name_file2
    echo '/build/debug/clbalancer 1 $dev 6 0 1024 $chunk_size 0.52 21' &>>$name_file
    sleep 2 
    echo $sample
  done
  done
done 

grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2


