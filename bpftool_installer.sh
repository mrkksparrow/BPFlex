#!/bin/bash
#sudo apt-get install -y clang
#sudo apt-get install -y llvm
#sudo apt-get install -y libelf-dev
#sudo apt-get install -y libc6-dev-i386
#sudo apt-get install -y gcc-multilib
#sudo apt install -y libbpf-dev
#sudo apt-get install -y libcap-dev

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
