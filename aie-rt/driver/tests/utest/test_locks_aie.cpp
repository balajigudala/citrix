#include "CppUTest/TestHarness.h"
#include <hw_config.h>

#if AIE_GEN == 1
static XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR,
		XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
		XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
		XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
		XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

static XAie_InstDeclare(DevInst, &ConfigPtr);

TEST_GROUP(LockApisAie)
{
	void setup()
	{
		AieRC RC;

		RC = XAie_CfgInitialize(&(DevInst), &ConfigPtr);
		CHECK_EQUAL(XAIE_OK, RC);

		RC = XAie_PartitionInitialize(&(DevInst), NULL);
		CHECK_EQUAL(XAIE_OK, RC);
	}
	void teardown()
	{
		AieRC RC;

		RC = XAie_PartitionTeardown(&(DevInst));
		CHECK_EQUAL(XAIE_OK, RC);

		XAie_Finish(&DevInst);
	}
};

TEST(LockApisAie, AcquireLock)
{
	AieRC RC;

	XAie_LocType TileLoc = XAie_TileLoc(8, 3);

	RC = XAie_LockRelease(&DevInst, TileLoc, XAie_LockInit(5, 1), 0);
	CHECK_EQUAL(XAIE_OK, RC);

	RC = XAie_LockAcquire(&DevInst, TileLoc, XAie_LockInit(5, 1), 0);
	CHECK_EQUAL(XAIE_OK, RC);

	RC = XAie_LockAcquire(&DevInst, TileLoc, XAie_LockInit(5, 1), 0);
	CHECK_EQUAL(XAIE_LOCK_RESULT_FAILED, RC);

	RC = XAie_LockRelease(&DevInst, TileLoc,
			XAie_LockInit(5, XAIE_LOCK_WITH_NO_VALUE), 0);
	CHECK_EQUAL(XAIE_OK, RC);

	RC = XAie_LockAcquire(&DevInst, TileLoc,
			XAie_LockInit(5, XAIE_LOCK_WITH_NO_VALUE), 0);
	CHECK_EQUAL(XAIE_OK, RC);

	RC = XAie_LockAcquire(&DevInst, TileLoc,
			XAie_LockInit(5, XAIE_LOCK_WITH_NO_VALUE), 0);
	CHECK_EQUAL(XAIE_LOCK_RESULT_FAILED, RC);
}

TEST(LockApisAie, ReleaseLock)
{
	AieRC RC;

	XAie_LocType TileLoc = XAie_TileLoc(8, 3);

	RC = XAie_LockRelease(&DevInst, TileLoc,
			XAie_LockInit(5, XAIE_LOCK_WITH_NO_VALUE), 0);
	CHECK_EQUAL(XAIE_OK, RC);

	RC = XAie_LockRelease(&DevInst, TileLoc, XAie_LockInit(5, 1), 0);
	CHECK_EQUAL(XAIE_OK, RC);
}
#endif
