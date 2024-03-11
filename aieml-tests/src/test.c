#include <stdio.h>
#include <xaiengine.h>

#include "hw_config.h"

#ifdef JENKINS_TESTS

#include <aiert_tests.h>

#endif

#define SIZE 256

/* List of all tests */
extern int test_aie_dma(XAie_DevInst *DevInst);
extern int test_aie_dma_event(XAie_DevInst *DevInst);
extern int test_aie_dma_fot(XAie_DevInst *DevInst);
extern int test_mem_tile_dma_fot(XAie_DevInst *DevInst);
extern int test_aie_data_mem(XAie_DevInst *DevInst);
extern int test_aie_dmabd_iter(XAie_DevInst *DevInst);
extern int test_mem_tile_bditer(XAie_DevInst *DevInst);
extern int test_aie_dma_task_complete_token(XAie_DevInst *DevInst);
extern int test_aie_mem_tiles(XAie_DevInst *DevInst);
extern int test_mem_tiles_compression(XAie_DevInst *DevInst);
extern int test_aie_dma_compression(XAie_DevInst *DevInst);
extern int test_aie_dma_tlast(XAie_DevInst *DevInst);
extern int test_mem_tile_dma_tlast(XAie_DevInst *DevInst);
extern int test_aie_tct_pl_fifo(XAie_DevInst *DevInst);
extern int test_aie_shimdma_loopback(XAie_DevInst *DevInst);
extern int test_aie_generate_intr(XAie_DevInst *DevInst);
extern int test_aie_col_status(XAie_DevInst *DevInst);
extern int test_aie_load_uc_elf(XAie_DevInst *DevInst);
extern int test_aie_ucdma_pause(XAie_DevInst *DevInst);
extern int test_aie_nocdma_pause(XAie_DevInst *DevInst);
extern int test_aie_uc_aximmoutstanding_txn(XAie_DevInst *DevInst);
extern int test_aie_noc_aximmoutstanding_txn(XAie_DevInst *DevInst);

int (*tests[])(XAie_DevInst *DevInst) =
{
	test_aie_dma,
	test_aie_dma_event,
	test_aie_dma_fot,
	test_mem_tile_dma_fot,
	test_aie_data_mem,
	test_aie_dmabd_iter,
	test_mem_tile_bditer,
	test_aie_dma_task_complete_token,
	test_aie_mem_tiles,
	test_mem_tiles_compression,
	test_aie_dma_compression,
	test_aie_dma_tlast,
	test_mem_tile_dma_tlast,
#if DEVICE != 0
	test_aie_tct_pl_fifo,
	test_aie_shimdma_loopback,
#endif /* DEVICE */
	test_aie_generate_intr,
	test_aie_col_status,
#if AIE_GEN == 5
	test_aie_load_uc_elf,
	test_aie_ucdma_pause,
	test_aie_nocdma_pause,
	test_aie_uc_aximmoutstanding_txn,
	test_aie_noc_aximmoutstanding_txn,
#endif
};

const char *test_names[] =
{
	"test_aie_dma",
	"test_aie_dma_event",
	"test_aie_dma_fot",
	"test_mem_tile_dma_fot",
	"test_aie_data_mem",
	"test_aie_dmabd_iter",
	"test_mem_tile_bditer",
	"test_aie_dma_task_complete_token",
	"test_aie_mem_tiles",
	"test_mem_tiles_compression",
	"test_aie_dma_compression",
	"test_aie_dma_tlast",
	"test_mem_tile_dma_tlast",
#if DEVICE != 0
	"test_aie_tct_pl_fifo",
	"test_aie_shimdma_loopback",
#endif /* DEVICE */
	"test_aie_generate_intr",
	"test_aie_col_status",
#if AIE_GEN == 5
	"test_aie_load_uc_elf",
	"test_aie_ucdma_pause",
	"test_aie_nocdma_pause",
	"test_aie_uc_aximmoutstanding_txn",
	"test_aie_noc_aximmoutstanding_txn",
#endif
};

static int search_test(const char **arr, int len, char *target) {

	for(int i = 0; i < len; i++) {
		if(strncmp(arr[i], target, strlen(target)) == 0) {
			return i;
		}
	}
	/* Return -1 when there is no match. */
	return -1;
}

int main(int argc, char **argv)
{
	AieRC RC;
	int ret = 0;
	int pass_count = 0;
	int num_tests = 0;
	int num_tests_aiert = 0;
	int total_tests_aiert = 0;
	int total_tests = sizeof(tests)/sizeof(tests[0]);
#ifdef JENKINS_TESTS
	total_tests_aiert = sizeof(tests_aiert)/sizeof(tests_aiert[0]);
#endif
	char input[SIZE] = {0};
	int index = 0;
	int index_aiert = 0;

	XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	XAie_InstDeclare(DevInst, &ConfigPtr);

	RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
	if(RC != XAIE_OK) {
		printf("Driver initialization failed.\n");
		return -1;
	}

start:
	/* Reset all variables */
	pass_count = 0;
	num_tests = 0;
	num_tests_aiert = 0;
	index = 0;
	index_aiert = 0;

        if(argc > 1) {
                if(strcmp(argv[1], "-a") == 0) {
                        printf("Executing all test cases\n");
                        num_tests = total_tests;
			num_tests_aiert = total_tests_aiert;
		}
        } else {
                printf("Specify any of the below option:\n");
                printf("\tl - list all test cases\n");
                printf("\tall - execute all test cases\n");
                printf("\t<test_name> - execute only specified test\n");
                printf("\tq - to quit\n");
                scanf("%s", input);

                if(strcmp(input, "l") == 0){
                        for(int i = 0; i < total_tests; i++) {
                                printf("%s\n", test_names[i]);
                        }
#ifdef JENKINS_TESTS
			for(int i = 0; i < total_tests_aiert; i++) {
                               	printf("%s\n", test_names_aiert[i]);
                        }
#endif
                        printf("End of Test cases List\n");
                        goto start;
                } else if(strcmp(input, "all") == 0) {
                        printf("Executing all test cases\n");
                        num_tests = total_tests;
			num_tests_aiert = total_tests_aiert;
                } else if(strcmp(input, "q") == 0) {
                        printf("Quitting...\n");
                        return 0;
                } else {
                        /* Search for the test case name in list. */
                        index = search_test(test_names, total_tests, input);
#ifdef JENKINS_TESTS
			index_aiert = search_test(test_names_aiert, total_tests_aiert, input);
#endif

			/* -1 is returned when there is no match*/
                        if(index == -1 && index_aiert == -1) {
				printf("Invalid input. Retry...\n");
                                goto start;
                        }

			if (index != -1)
                        	num_tests = 1;
			else if (index_aiert != -1)
				num_tests_aiert = 1;
                }
        }

	if(DevInst.Backend->Type == XAIE_IO_BACKEND_BAREMETAL) {
		RC = XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
		if(RC != XAIE_OK) {
			printf("Failed to update NPI address\n");
			return -1;
		}
	}

	/*
	 * Execute AIEML tests defined in the aieml-tests repository
	 */
	for(uint32_t i = 0; i < num_tests; i++) {

		if(num_tests != 1 ) {
			index = i;
		}

		pass_count++;
		RC = XAie_PartitionInitialize(&DevInst, NULL);
		if(RC != XAIE_OK) {
			printf("Partition initialization failed for %s.\n",
					test_names[index]);
			return -1;
		}

		ret = (*tests[index])(&DevInst);
		if(ret) {
			pass_count--;
			printf("%s failed.\n", test_names[i]);
		}
#if DEVICE != 0
		RC = XAie_PartitionTeardown(&DevInst);
		if(RC != XAIE_OK) {
			printf("Partition teardown failed for %s.\n",
					test_names[i]);
			return -1;
		}
#endif  /* DEVICE */
	}

	/*
	 * Execute AIEML tests defined in the aie-rt repository as part of the testing-infra
	 */
#ifdef JENKINS_TESTS
	for(uint32_t i = 0; i < num_tests_aiert; i++) {

                if(num_tests_aiert != 1 ) {
                        index_aiert = i;
                }

                pass_count++;
                RC = XAie_PartitionInitialize(&DevInst, NULL);
                if(RC != XAIE_OK) {
                        printf("Partition initialization failed for %s.\n",
                                        test_names_aiert[index_aiert]);
                        return -1;
                }

                ret = (*tests_aiert[index_aiert])(&DevInst);
                if(ret) {
                        pass_count--;
                        printf("%s failed.\n", test_names_aiert[i]);
                }
#if DEVICE != 0
                RC = XAie_PartitionTeardown(&DevInst);
                if(RC != XAIE_OK) {
                        printf("Partition teardown failed for %s.\n",
                                        test_names_aiert[i]);
                        return -1;
                }
#endif  /* DEVICE */
        }
#endif
	int total_executed_tests = num_tests + num_tests_aiert;
	if(pass_count != total_executed_tests) {
		printf("%d/%d AIEML tests passed\n", pass_count, total_executed_tests);
		return -1;
	}

	printf("%d/%d AIEML tests passed.\n", pass_count, total_executed_tests);
	printf("Tests Execution Completed\n");

        if(argc > 1) {
                if(strcmp(argv[1], "-a") == 0) {
                        return 0;
                }
        }

	goto start;
}
