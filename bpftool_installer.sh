#!/bin/bash
sudo apt-get install clang
sudo apt-get install llvm
sudo apt-get install libelf-dev
sudo apt-get install libc6-dev-i386
sudo apt-get install -y gcc-multilib
sudo apt install libbpf-dev
sudo apt-get install libcap-dev

# Go to src/cc directory
cd src/cc

# Clone libbpf directory
git clone https://github.com/libbpf/libbpf.git

# Go to libbpflex directory
cd ../../libbpflex

# Clone bpftool 
git clone https://github.com/libbpf/bpftool.git

# Go to the bpftool directory
cd bpftool

# Delete the libbpf directory within bpftool
rm -rf libbpf

# Clone libbpf again 
git clone https://github.com/libbpf/libbpf.git


cd ../../
#
