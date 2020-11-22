make
cd ./build

pintos -k -T 60 --bochs  --filesys-size=2 -p tests/userprog/exec-missing -a exec-missing -- -q  -f run exec-missing < /dev/null 2> tests/userprog/exec-missing.errors > tests/userprog/exec-missing.output
perl -I../.. ../../tests/userprog/exec-missing.ck tests/userprog/exec-missing tests/userprog/exec-missing.result

cd ..
