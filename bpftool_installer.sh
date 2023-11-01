#!/bin/bash

sudo apt-get install libelf-dev
sudo apt-get install libc6-dev-i386
sudo apt-get install -y gcc-multilib
sudo apt install libbpf-dev

# Go to src/cc directory
cd src/cc

# Clone a directory (replace with the actual repository URL and directory name)
git clone https://github.com/libbpf/libbpf.git

# Go to libbpflex directory
cd ../../libbpflex

# Clone bpftool (replace with the actual repository URL)
git clone https://github.com/libbpf/bpftool.git

# Go to the bpftool directory
cd bpftool

# Delete the libbpf directory within bpftool
rm -rf libbpf

# Clone libbpf again (replace with the actual repository URL)
git clone https://github.com/libbpf/libbpf.git

# Return to the main directory
cd ../../
#
