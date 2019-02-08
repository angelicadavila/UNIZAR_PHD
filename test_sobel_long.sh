path=test_sobel/grendel
name_file=$path/hguided_ 
name_file2=$path/hguidedt_ 

for dev in 3 5 6 7 
 do
   for sample in {1..5}
   do
   let _size=`expr 312595200*6*10`
   let chunk_size=1024
   echo $_size
     echo $dev
    ./build/debug/EngineCL 2 $dev 5 0 1024 $chunk_size 0.52 21 &>>$name_file$dev.txt
     (tail -n2) < $name_file$dev.txt
     (tail -n2) < $name_file$dev.txt>>$name_file2
     echo '/build/debug/EngineCL 2 $dev 5 0 size 10_size 0.133 21' &>>$name_file$dev.txt
     sleep 5
    echo $sample
   done
 grep -v "Done" $name_file2 > file2.txt; mv file2.txt $name_file2$dev.txt
done 
