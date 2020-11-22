make
cd ./build
pintos -k -T 60 --bochs  --filesys-size=2 -p tests/filesys/base/syn-write -a syn-write -p tests/filesys/base/child-syn-wrt -a child-syn-wrt -- -q  -f run syn-write < /dev/null 2> tests/filesys/base/syn-write.errors > tests/filesys/base/syn-write.output
perl -I../.. ../../tests/filesys/base/syn-write.ck tests/filesys/base/syn-write tests/filesys/base/syn-write.result
cd ..
