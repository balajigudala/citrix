source /proj/xsjsswstaff/sankarji/aie-arch/aie-arch-env-files/aie2ps-arch.sh
export LD_LIBRARY_PATH=$PWD/aie-rt/driver/src:$LD_LIBRARY_PATH
cd ./build_systemc/
pkill mesimulator
mesimulator -d ${ME_ROOT}/data/aie2ps_5x4_1ms.json --microblaze-uart-to-stdout -r &
