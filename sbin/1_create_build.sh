#!/bin/bash

cd ../

. script.env

rm -rf build
mkdir -p build


#cmake . -B build -DCMAKE_VERBOSE_MAKEFILE=1
cmake . -B build
