make
cd ./build

pintos -k -T 360 --bochs  --filesys-size=2 -p tests/userprog/no-vm/multi-oom -a multi-oom -- -q  -f run multi-oom < /dev/null 2> tests/userprog/no-vm/multi-oom.errors > tests/userprog/no-vm/multi-oom.output
perl -I../.. ../../tests/userprog/no-vm/multi-oom.ck tests/userprog/no-vm/multi-oom tests/userprog/no-vm/multi-oom.result

cd ..
