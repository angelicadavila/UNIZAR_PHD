name_file=test_watermark/static_
name_file2=test_watermark/statict_

for dev in 4 2  
do
  for sample in {1..10}
  do
#    echo $dev
    ./build/debug/EngineCL 1 $dev 6 0 19537200 19537200 0.52 21 &>>$name_file$dev.txt
    (tail -n2) < $name_file$dev.txt
    (tail -n2) < $name_file$dev.txt>>$name_file2
    echo '/build/debug/clbalancer 0' $dev '6 0 19537200  19537200 0.52 21' &>>$name_file$dev.txt
    sleep 5
    echo $sample
    
  done
	grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2$dev.txt
done 

for dev in 1 
do
  for sample in {1..10}
  do
#    echo $dev
    ./build/debug/EngineCL 1 $dev 6 0 19537200 1862500 0.52 21 &>>$name_file$dev.txt
    (tail -n2) < $name_file$dev.txt
    (tail -n2) < $name_file$dev.txt>>$name_file2
    echo '/build/debug/clbalancer 0' $dev '6 0 19537200  19537200 0.52 21' &>>$name_file$dev.txt
    sleep 5
    echo $sample
    
  done
	grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2$dev.txt
done 

#
#name_file=test_watermark/data21_dynamic.txt
#name_file2=test_watermark/data21t_dynamic.txt
#
#for dev in 4 2 1 7 6 5 3
#do
#  for sample in {1..20}
#  do
#    echo $dev
#    ./build/debug/clbalancer 1 $dev 6 0 1024 207360 0.52 21 &>>$name_file
#    (tail -n2) < $name_file
#    (tail -n2) < $name_file>>$name_file2
#    echo '/build/debug/clbalancer 1 $dev 6 0 1024 207360 0.52 21' &>>$name_file
#    sleep 5
#    echo $sample
#  done
#done 
#
#grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2
