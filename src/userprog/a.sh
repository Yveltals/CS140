make
cd ./build
pintos -k -T 300 --bochs  --filesys-size=2 -p tests/filesys/base/syn-read -a syn-read -p tests/filesys/base/child-syn-read -a child-syn-read -- -q  -f run syn-read < /dev/null 2> tests/filesys/base/syn-read.errors > tests/filesys/base/syn-read.output
perl -I../.. ../../tests/filesys/base/syn-read.ck tests/filesys/base/syn-read tests/filesys/base/syn-read.result
cd ..
