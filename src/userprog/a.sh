make
cd ./build
pintos -k -T 60 --bochs  --filesys-size=2 -p tests/userprog/exec-bound-3 -a exec-bound-3 -- -q  -f run exec-bound-3 < /dev/null 2> tests/userprog/exec-bound-3.errors > tests/userprog/exec-bound-3.output
perl -I../.. ../../tests/userprog/exec-bound-3.ck tests/userprog/exec-bound-3 tests/userprog/exec-bound-3.result
cd ..
