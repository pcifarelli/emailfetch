#!/bin/bash
cd ${WORKSPACE}

# clean up artifacts
rm -rf artifacts/

# make sure the configure script has been built
./build.sh

# configure
./configure --enable-docker --prefix=${WORKSPACE}/artifacts/

# make
make clean && make && make install

