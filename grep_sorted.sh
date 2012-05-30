#! /bin/sh

sorted=$(ls -1 /var/lib/kvalobs/synopreports/msys2_*)

for file in $sorted; do
    grep $@ $file
done
