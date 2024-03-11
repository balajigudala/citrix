PREFIX ?= aarch64-linux-gnu-
CC = gcc
CXX = g++
TOOLS_VERSION = $(shell which v++ | sed 's/.*Vitis\///;s/\/bin.*//')

PROJ_ROOT = $(PWD)
AIE_DRIVER ?= aie-rt/driver
AIE_FAL ?= aie-rt/fal/src
INCLUDE = $(AIE_DRIVER)/include
AIERT_TESTS_INCLUDE = $(AIE_DRIVER)/tests/stest/
AIERT_TESTS_INCLUDE_MAINAIE = $(AIE_DRIVER)/test/stest/
LD_PATH = $(AIE_DRIVER)/src
ifeq ($(AIE_GEN),5)
SRCS = $(wildcard src/*.c src/aie2ps/*.c)
else
SRCS = $(wildcard src/*.c)
endif
FAL_SRCS = $(wildcard fal-src/*.cpp)

#Device 0 will be systemC simulator for every AIE_GEN
AIE_GEN ?= 2

LINUX_ARTIFACTS_PATH = /proj/xbuilds/$(TOOLS_VERSION)_daily_latest/internal_platforms/sw/versal/xilinx-versal
ifeq "$(AIE_GEN)" "1"
	#Default s80 vck190
	DEVICE ?= 80
	EMU_PLAT ?= /proj/xbuilds/$(TOOLS_VERSION)_daily_latest/internal_platforms/xilinx_vck190_base_$(strip $(subst .,, $(TOOLS_VERSION)))0_1/hw_emu/hw_emu.xsa
	XSA ?= /proj/xbuilds/$(TOOLS_VERSION)_daily_latest/internal_platforms/xilinx_vck190_base_$(strip $(subst .,, $(TOOLS_VERSION)))0_1/hw/hw.xsa
	SIM_PLAT ?= ${ME_ROOT}/data/me_5x4.json
else ifeq "$(AIE_GEN)" "2"
	#Default sv60 vek280
	DEVICE ?= 60
	EMU_PLAT ?= /proj/xbuilds/$(TOOLS_VERSION)_daily_latest/internal_platforms/xilinx_vek280_base_$(strip $(subst .,, $(TOOLS_VERSION)))0_1/hw_emu/hw_emu.xsa
	XSA ?= /proj/xbuilds/$(TOOLS_VERSION)_daily_latest/internal_platforms/xilinx_vek280_base_$(strip $(subst .,, $(TOOLS_VERSION)))0_1/hw/hw.xsa
	SIM_PLAT ?= ${ME_ROOT}/data/aie2_5x4_device.json
else ifeq "$(AIE_GEN)" "5"
	#Default t50 IPP platform
	DEVICE ?= 80
	XSA ?= /everest/siv_rel_bkup/proj/telluride/public/releases/ipp/aie_ipp/aie2ps_ipp_v2_0/xsa/ps0_me_wrapper.xsa
	SIM_PLAT ?= ${ME_ROOT}/data/aie2ps_5x4_1ms.json
endif

ifeq ($(JENKINS_TESTS),yes)
  AIERT_TEST_SRCS = $(wildcard $(AIE_DRIVER)/tests/stest/*.c)
  AIERT_TEST_SRCS_MAINAIE = $(wildcard $(AIE_DRIVER)/test/stest/*.c)
endif

.PHONY: clean

all : driver systemc

sim : systemc run_sim

check_tools:
	@if [ -z $(strip $(TOOLS_VERSION)) ]; then echo "Must source Vitis tools to build"; exit 1; fi

check_xsa:
	@if [ -z $(strip $(XSA)) ]; then echo "No Hardware XSA defined"; exit 1; fi

check_emu:
	@if [ -z $(strip $(EMU_PLAT)) ]; then echo "No emulation XSA defined"; exit 1; fi

check_sim:
	@if [ -f $(strip $(SIM_PLAT)) ]; then echo "Systemc simulation file not found"; exit 1; fi

#Baremetal builds
baremetal: check_tools check_xsa
	mkdir -p build_baremetal
	xsct aieml_tests.tcl $(XSA) $(DEVICE) $(AIE_GEN) $(LD_PATH)

fal_baremetal: check_tools check_xsa
	mkdir -p build_fal_baremetal
	xsct aieml_fal_tests.tcl $(XSA) $(DEVICE) $(AIE_GEN) $(LD_PATH) $(AIE_FAL)

emu_baremetal: baremetal
	mkdir -p build_emu_baremetal
	cd build_emu_baremetal; \
	v++ -s -l -t hw_emu -f ${EMU_PLAT} -o vail.xsa; \
	v++ -s -p -t hw_emu --platform ${EMU_PLAT} --package.domain aiengine --package.boot_mode sd --package.image_format=fat32 --package.ps_elf ${PROJ_ROOT}/build_baremetal/aieml_vnc_tests/Debug/aieml_vnc_tests.elf,a72-0 vail.xsa -o temp.xclbin --package.out_dir baremetal.package.hw_emu; \
	cd .. ;

run_emu_baremetal: emu_baremetal
	cd build_emu_baremetal; \
	cd baremetal.package.hw_emu;
	./launch_hw_emu.sh ;\
	cd ../../ ;

#Linux builds
linux_driver:
	make -C $(AIE_DRIVER)/src CC=$(PREFIX)$(CC) CFLAGS='-Wall -g -Wextra --std=c11 -D__AIELINUX__' -f Makefile.Linux

linux: linux_driver
	mkdir -p build_linux
	$(PREFIX)$(CC) --std=c11 -I$(INCLUDE) -o build_linux/aieml_tests $(SRCS) -L$(LD_PATH) \
		-lxaiengine -DDEVICE=$(DEVICE) -DAIE_GEN=$(AIE_GEN)

fal_linux: linux_driver
	mkdir -p build_fal_linux/include
	cp -r $(AIE_FAL) build_fal_linux/include/xaiefal
	$(PREFIX)$(CXX) -I$(INCLUDE) -Ibuild_fal_linux/include -o build_fal_linux/aieml_fal_tests $(FAL_SRCS) -L$(LD_PATH) \
		-lxaiengine -DDEVICE=$(DEVICE) -DAIE_GEN=$(AIE_GEN)

emu_linux: check_emu check_tools
	mkdir -p build_emu_linux
	cd build_emu_linux; \
	v++ -s -l -t hw_emu -f ${EMU_PLAT} -o vail.xsa; \
	v++ -s -p -t hw_emu --platform ${EMU_PLAT} --package.boot_mode sd --package.rootfs ${LINUX_ARTIFACTS_PATH}/rootfs.ext4 --package.sd_file ${LINUX_ARTIFACTS_PATH}/Image vail.xsa --package.out_dir linux.package.hw_emu; \
	cd .. ;

#SystemC Builds
systemc_driver:
	make -C $(AIE_DRIVER)/src CC=$(CC) CFLAGS='-Wall -g -Wextra --std=c11 -D__AIESOCKET__' -f Makefile.Linux

ucfw_exe.o:
	bash -c "cd ./src/aie2ps/ucfirmware;source ./env.sh;make clean;make"
	bash -c "cd ./src/aie2ps/ucfirmware/output; ld -EL -r -b binary -o ucfw_exe.o ./app.elf"

ifeq ($(AIE_GEN),5)
systemc: systemc_driver ucfw_exe.o
	mkdir -p build_systemc
ifeq ($(JENKINS_TESTS),yes)
		$(CC) --std=c11 -DJENKINS_TESTS -I$(INCLUDE) -I$(AIERT_TESTS_INCLUDE) \
			-I$(AIERT_TESTS_INCLUDE_MAINAIE) -o build_systemc/aieml_tests \
			./src/aie2ps/ucfirmware/output/ucfw_exe.o \
			$(SRCS) $(AIERT_TEST_SRCS) $(AIERT_TEST_SRCS_MAINAIE) -L$(LD_PATH) \
                -lxaiengine -DDEVICE=0 -DAIE_GEN=$(AIE_GEN) -D_UCFW_
else
	        $(CC) --std=c11 -I$(INCLUDE) -g -o build_systemc/aieml_tests \
			./src/aie2ps/ucfirmware/output/ucfw_exe.o \
			$(SRCS) -L$(LD_PATH) -lxaiengine -DDEVICE=0 -DAIE_GEN=$(AIE_GEN) -D_UCFW_
endif
else
systemc: systemc_driver
	mkdir -p build_systemc
ifeq ($(JENKINS_TESTS),yes)
		$(CC) --std=c11 -DJENKINS_TESTS -I$(INCLUDE) -I$(AIERT_TESTS_INCLUDE) \
			-I$(AIERT_TESTS_INCLUDE_MAINAIE) -o build_systemc/aieml_tests \
			$(SRCS) $(AIERT_TEST_SRCS) $(AIERT_TEST_SRCS_MAINAIE) -L$(LD_PATH) \
			-lxaiengine -DDEVICE=0 -DAIE_GEN=$(AIE_GEN)
else
	        $(CC) --std=c11 -I$(INCLUDE) -g -o build_systemc/aieml_tests \
			$(SRCS) -L$(LD_PATH) -lxaiengine -DDEVICE=0 -DAIE_GEN=$(AIE_GEN)

endif
endif

run_sim:
	echo 'running systemC'
	cd build_systemc; \
	(mesimulator -d $(SIM_PLAT) -r &) ; \
	sleep 10; \
	./aieml_tests; \
	cd ..;

run_sim_all:
	echo 'running systemC'
	cd build_systemc; \
	(mesimulator -d $(SIM_PLAT) -r &) ; \
	sleep 10; \
	./aieml_tests -a; \
	cd ..;

driver_clean:
	make -C $(AIE_DRIVER)/src -f Makefile.Linux clean

clean:
	rm -rf build_*/ include
