# BPFlex
   An eBPF framework designed for deploying eBPF programs in native C.

Steps:

    git clone https://github.com/karthickpython/BPFlex.git

    cd BPFlex
    
    sudo bash bpftool_installer.sh
    
    cd libbpflex
    
    make
    
    sudo ./tcpstats

View unique host connections in 'unique.txt' within the same directory.

Example output:

 Time: 16:42:56 Process Name: Chrome_ChildIOT    S_IP: 10.15.202.91    SPort: 56922   D_IP: 136.143.180.20   DPort: 443    ThreadID: 2146    PID: 2125     Latency: 291.224
 Time: 16:43:10 Process Name: irq/150-iwlwifi    S_IP: 10.15.202.91    SPort: 59442   D_IP: 136.143.191.162   DPort: 443    ThreadID: 556    PID: 556     Latency: 254.175
 Time: 16:43:46 Process Name: irq/153-iwlwifi    S_IP: 10.15.202.91    SPort: 55034   D_IP: 136.143.186.20   DPort: 443    ThreadID: 559    PID: 559     Latency: 313.929
 Time: 16:44:16 Process Name: Chrome_ChildIOT    S_IP: 10.15.202.91    SPort: 56672   D_IP: 27.123.42.205   DPort: 443    ThreadID: 2146    PID: 2125     Latency: 23.980
 Time: 16:44:17 Process Name: Chrome_ChildIOT    S_IP: 10.15.202.91    SPort: 52538   D_IP: 136.143.191.101   DPort: 443    ThreadID: 2146    PID: 2125     Latency: 249.709

Time: 16:45:41 Process Name: Socket Thread    S_IP: 10.15.202.91    SPort: 45360   D_IP: 34.149.100.209   DPort: 443    ThreadID: 22059    PID: 21967     Latency: 3.114
 Time: 16:45:41 Process Name: Socket Thread    S_IP: 10.15.202.91    SPort: 57800   D_IP: 34.120.208.123   DPort: 443    ThreadID: 22059    PID: 21967     Latency: 3.698
 Time: 16:45:41 Process Name: Socket Thread    S_IP: 10.15.202.91    SPort: 56440   D_IP: 34.117.237.239   DPort: 443    ThreadID: 22059    PID: 21967     Latency: 3.115
 Time: 16:45:41 Process Name: Socket Thread    S_IP: 10.15.202.91    SPort: 54378   D_IP: 34.120.115.102   DPort: 443    ThreadID: 22059    PID: 21967     Latency: 4.543


