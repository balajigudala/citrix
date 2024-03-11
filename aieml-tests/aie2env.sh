source /proj/xsjsswstaff/sankarji/aie-arch/aie-arch-env-files/aie2-arch.sh
export LD_LIBRARY_PATH=$PWD/aie-rt/driver/src:$LD_LIBRARY_PATH
cd ./build_systemc/
pkill mesimulator
mesimulator -d ${ME_ROOT}/data/aie2_5x4_device.json -r &
