# AIEML VnC tests

## Supported Builds

### Platforms

Supports 3 platforms.
1. Baremetal
2. Linux
3. SystemC simualtion

### Hardware generations
Supports builds for different hardware generations:

| Gen # | ARCH |
| --- | --- |
| 1 | AIE 1 |
| 2 | AIE 2 |
| 3 | AIEIPU |
| 4 | AIE2P |
| 5 | AIE2PS |

## Build instructions

### Source Vitis tools

Need to source Vitis and Petalinux tools for v++ and cross compiler

```
source /proj/xbuilds/2023.2_daily_latest/installs/lin64/Vitis/2023.2/settings64.sh
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
```

### Baremetal

Select hardware generation and run

```
make AIE_GEN=<1,2,3,4,5> baremetal
```

To build baremetal emulation platform, run
```
make AIE_GEN=<1,2,3,4,5> emu_baremetal
```

To automatically run tests in the emulation platform, run
```
make AIE_GEN=<1,2,3,4,5> run_emu_baremetal
```

### Linux

To compile the test cases for Linux, set CC variable appropriately and run
```
make AIE_GEN=<1,2,3,4,5> linux
```

To build linux emulation platform, run
```
make AIE_GEN=<1,2,3,4,5> emu_linux
```

### SystemC

To build systemc tests, as the aieml-test worked with aiedriver socket backend, the 
app will running on x86(host), then there is no requirment to source simulator environment
here for compile. and if any cross compile command already runned, the cross compile environment
exit is required to finish this system-c build.

```
make AIE_GEN=<1,2,3,4,5> systemc
```

Run the below command to source systemC simulation environment.

Source different sh file for different architectures

AI2
```
source /proj/xsjsswstaff/sankarji/aie-arch/aie-arch-env-files/aie2-arch.sh
export LD_LIBRARY_PATH=$PWD/aie-rt/driver/src:$LD_LIBRARY_PATH
```

AI2PS
```
source /proj/xsjsswstaff/sankarji/aie-arch/aie-arch-env-files/aie2ps-arch.sh
export LD_LIBRARY_PATH=$PWD/aie-rt/driver/src:$LD_LIBRARY_PATH
```

To automatically launch and run tests on simulator, run
```
make AIE_GEN=<1,2,3,4,5> run_sim
```

## Execute tests

### Test Options
```
l - list all tests cases
all - execute all test cases
<test_name> - executre only specified test
q - to quit
```

To skip the menu and to automatically execute all tests, pass the -a flag.
```
## Real time debug with gdb


./aieml_tests -a
```

## Real time debug with GDB

aie2ps
```
bash
make AIE_GEN=5 systemc
source ./aie2psenv.sh
gdb ./aieml_test
```
