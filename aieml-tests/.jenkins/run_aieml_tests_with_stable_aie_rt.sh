#!/bin/bash
# Environment variables
BRANCH_NAME=${GITHUB_PR_SOURCE_BRANCH}
GIT_URL="https://gitenterprise.xilinx.com/${GITHUB_PR_SOURCE_REPO_OWNER}/aieml-tests.git"
PR_NUMBER=${GITHUB_PR_NUMBER}

# Run make command for tests
echo "Do you see the following two environment variables???" 
echo $BRANCH_NAME
echo $GIT_URL
echo "Did you see the above two environment variables???" 


# checkout latest aieml-tests directory 
git clone https://gitenterprise.xilinx.com/ai-engine/aieml-tests.git
cd aieml-tests
git checkout $BRANCH_NAME
git submodule update --init --recursive
#source /proj/xbuilds/HEAD_INT_daily_latest/installs/lin64/Vitis/HEAD/settings64.sh
source /proj/xbuilds/HEAD_qualified_latest/installs/lin64/Vitis/HEAD/settings64.sh
make driver
make systemc
source /proj/xsjsswstaff/sankarji/aie-arch/aie-arch-env-files/aie2-arch.sh
export LD_LIBRARY_PATH=$PWD/aie-rt/driver/src/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/scratch/staff/weiwang/:$LD_LIBRARY_PATH
make run_sim_all
cd ..
