# ePPing PAM 2023 build

This repository contains the code for the ePPing application that was
used for the PAM 2023 paper "Efficient continuous latency monitoring
with eBPF". For the most current build of ePPing please see
[xdp-project/bpf-examples](https://github.com/simosund/bpf-examples/tree/master/pping)
instead. The code for ePPing is usually bundled with many other cool XDP
applications, but for this repository I have removed them to keep it
focused on ePPing.

Two different builds of ePPing were used for the experiments in the
paper. The build in the [main
branch](https://github.com/simosund/ePPing-PAM23/tree/main) was used
for the performance tests, whereas the build in the [additional-output
branch](https://github.com/simosund/ePPing-PAM23/tree/additional-output)
(adds TSecr and ACK number to each RTT report) was used for evaluating
the RTT accuracy and showcasing the difference between matching TCP
timestamps vs sequence and ACK numbers.

## Build
To build ePPing, run the following commands
```
git submodule update --init
CLANG=clang-14 LLC=llc-14 ./configure
cd pping
make
```

Note that for the experiments ePPing was built with LLVM-14 (clang-14
and llc-14). This build will fail to run if compiled with LLVM-15+, as
the fix for working with LLVM-15+ was added later.

## Run
To run ePPing, run the following commands
```
cd pping
sudo ./pping -i <interface> [optional args]
```

To list the optional arguments, simply run `sudo ./pping -h`. A short
explanation of each argument can be found in pping/pping.c
