#!/bin/bash

if [ $# == 0 ]; then
    echo "Недостаточно аргументов"
    exit 1
fi

s=0
for ((i=1; i<=$#; i++)); do
    s=$((s + ${!i}))
done

echo $((s/$#))
