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


Obtain metrics for the number of unique connections, along with connection details, including the creating process name, process ID, thread ID, 5-tuple information, and connection establishment latency.


Example output:


 Time: 16:42:56 Process Name: Chrome_ChildIOT    S_IP: 10.15.202.91    SPort: 56922   D_IP: 136.143.180.20   DPort: 443    ThreadID: 2146    PID: 2125     Latency: 291.224

 
 Time: 16:43:10 Process Name: irq/150-iwlwifi    S_IP: 10.15.202.91    SPort: 59442   D_IP: 136.143.191.162   DPort: 443    ThreadID: 556    PID: 556     Latency: 254.175


Time: 16:45:41 Process Name: Socket Thread    S_IP: 10.15.202.91    SPort: 45360   D_IP: 34.149.100.209   DPort: 443    ThreadID: 22059    PID: 21967     Latency: 3.114



