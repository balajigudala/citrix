#include <stdio.h>
#include <xaiengine.h>

#include "hw_config.h"

#define SIZE 256

/* List of all tests */
extern int test_aiefal_aietile_pcevent(XAie_DevInst *DevInst);
extern int test_aiefal_aietile_pcrange(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_broadcast(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_comboevent(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_groupevent(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_perfcounter(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_perfcounter_event(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_streamswitch(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_trace(XAie_DevInst *DevInst);
extern int test_aiefal_memtile_userevent(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_broadcast(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_comboevent(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_perfcounter(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_perfcounter_event(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_streamswitch(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_trace(XAie_DevInst *DevInst);
extern int test_aiefal_shimtile_userevent(XAie_DevInst *DevInst);
extern int test_aiefal_timersync(XAie_DevInst *DevInst);

int (*tests[])(XAie_DevInst *DevInst) =
{
	test_aiefal_aietile_pcevent,
	test_aiefal_aietile_pcrange,
	test_aiefal_memtile_broadcast,
	test_aiefal_memtile_comboevent,
	test_aiefal_memtile_groupevent,
	test_aiefal_memtile_perfcounter,
	test_aiefal_memtile_perfcounter_event,
	test_aiefal_memtile_streamswitch,
	test_aiefal_memtile_trace,
	test_aiefal_memtile_userevent,
	test_aiefal_shimtile_broadcast,
	test_aiefal_shimtile_comboevent,
	test_aiefal_shimtile_perfcounter,
	test_aiefal_shimtile_perfcounter_event,
	test_aiefal_shimtile_streamswitch,
	test_aiefal_shimtile_trace,
	test_aiefal_shimtile_userevent,
	test_aiefal_timersync,
};

const char *test_names[] =
{
	"test_aiefal_aietile_pcevent",
	"test_aiefal_aietile_pcrange",
	"test_aiefal_memtile_broadcast",
	"test_aiefal_memtile_comboevent",
	"test_aiefal_memtile_groupevent",
	"test_aiefal_memtile_perfcounter",
	"test_aiefal_memtile_perfcounter_event",
	"test_aiefal_memtile_streamswitch",
	"test_aiefal_memtile_trace",
	"test_aiefal_memtile_userevent",
	"test_aiefal_shimtile_broadcast",
	"test_aiefal_shimtile_comboevent",
	"test_aiefal_shimtile_perfcounter",
	"test_aiefal_shimtile_perfcounter_event",
	"test_aiefal_shimtile_streamswitch",
	"test_aiefal_shimtile_trace",
	"test_aiefal_shimtile_userevent",
	"test_aiefal_timersync",
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
	uint32_t pass_count = 0;
	uint32_t num_tests = 0;
	int total_tests = sizeof(tests)/sizeof(tests[0]);
	char input[SIZE] = {0};
	int index = 0;

	XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	XAie_InstDeclare(DevInst, &ConfigPtr);

start:
	/* Reset all variables */
	pass_count = 0;
	num_tests = 0;
	index = 0;

        if(argc > 1) {
                if(strcmp(argv[1], "-a") == 0) {
                        printf("Executing all test cases\n");
                        num_tests = total_tests;
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
                        printf("End of Test cases List\n");
                        goto start;
                } else if(strcmp(input, "all") == 0) {
                        printf("Executing all test cases\n");
                        num_tests = total_tests;
                } else if(strcmp(input, "q") == 0) {
                        printf("Quitting...\n");
                        return 0;
                } else {
                        /* Search for the test case name in list. */
                        index = search_test(test_names, total_tests, input);
                        /* -1 is returned when there is no match */
                        if(index == -1 ) {
                                printf("Invalid input. Retry...\n");
                                goto start;
                        }
                        num_tests = 1;
                }
        }

	for(uint32_t i = 0; i < num_tests; i++) {

		if(num_tests != 1 ) {
			index = i;
		}

		pass_count++;

		RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
		if(RC != XAIE_OK) {
			printf("Driver initialization failed for %s.\n",
					test_names[i]);
			goto start;
		}
		if(DevInst.Backend->Type == XAIE_IO_BACKEND_BAREMETAL) {
			RC = XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
			if(RC != XAIE_OK) {
				printf("Failed to update NPI address for %s\n",
						test_names[i]);
				goto start;
			}
		}
		RC = XAie_PartitionInitialize(&DevInst, NULL);
		if(RC != XAIE_OK) {
			printf("Parition inialization failed for %s.\n",
					test_names[i]);
			goto start;
		}

		RC = XAie_PmRequestTiles(&DevInst, NULL, 0);
		if(RC != XAIE_OK) {
			printf("Request tiles failed for %s.\n",
					test_names[i]);
			goto start;
		}

		ret = (*tests[index])(&DevInst);
		if(ret) {
			pass_count--;
			printf("%s failed.\n", test_names[i]);
		}
		RC = XAie_PartitionTeardown(&DevInst);
		if(RC != XAIE_OK) {
			printf("Parition teardown failed for %s.\n",
					test_names[i]);
			goto start;
		}
		RC = XAie_Finish(&DevInst);
		if(RC != XAIE_OK) {
			printf("Driver finish failed for %s.\n",
					test_names[i]);
			goto start;
		}
	}

	if(pass_count != num_tests) {
		printf("%d/%d tests passed\n", pass_count, num_tests);
	}

	printf("%d/%d AIEML FAL tests passed.\n", pass_count, num_tests);
	printf("Tests Execution Completed\n");

        if(argc > 1) {
                if(strcmp(argv[1], "-a") == 0) {
                        return 0;
                }
        }

	goto start;
}
