name_file=test_watermark/data2_only.txt
name_file2=test_watermark/data2t_only.txt

for dev in 4 2 1
do
  for sample in {1..20}
  do
#    echo $dev
    ./build/debug/clbalancer 0 $dev 6 0 1024 207360 0.52 21 &>>$name_file
    (tail -n2) < $name_file
    (tail -n2) < $name_file>>$name_file2
    echo '/build/debug/clbalancer 0 $dev 6 0 1024 207360 0.52 21' &>>$name_file
    sleep 5
    echo $sample
    
  done
done 

grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2

name_file=test_sobel/data2_dynamic.txt
name_file2=test_sobel/data2t_dynamic.txt

for dev in 4 2 1 7 6 5 3
do
  for sample in {1..20}
  do
    echo $dev
    ./build/debug/clbalancer 1 $dev 6 0 1024 207360 0.52 21 &>>$name_file
    (tail -n2) < $name_file
    (tail -n2) < $name_file>>$name_file2
    echo '/build/debug/clbalancer 1 $dev 6 0 1024 207360 0.52 21' &>>$name_file
    sleep 5
    echo $sample
  done
done 

grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2
