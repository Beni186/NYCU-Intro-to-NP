#!/bin/bash

sudo mount -fav
touch ~/efs/a.txt ~/efs/b.txt ~/efs/c.txt
echo "0" > ~/efs/a.txt
echo "0" > ~/efs/b.txt
echo "0" > ~/efs/c.txt
mkdir -p build
make