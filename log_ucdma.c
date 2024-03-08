

/******* 140224 ******/
for dir  = 0
test_aie_UcDma_pause
/****** Pausing Ucdma ******/
Mask : 1
UM : 1
Lsb : 0
value : 1
fld value : 1
regaddr : c0120
reg data : 1
Mask : 1
UM : 1
Lsb : 0
value : 0
fld value : 0
regaddr : c0120
reg data : 0
Load UC elf.



/******* 140224 ******/
for dir = 1
test_aie_UcDma_pause
/****** Pausing Ucdma ******/
Mask : 2
UM : 2
Lsb : 1
value : 2
fld value : 2
regaddr : c0120
reg data : 2
Mask : 2
UM : 2
Lsb : 1
value : 0
fld value : 0
regaddr : c0120
reg data : 0
Load UC elf.


/******140224 *******/
for loc = NULL;
test_aie_UcDma_pause
/****** Pausing Ucdma ******/
no of cols :5
Loc addr :0
tile : 0
tile : 1
tile : 2
tile : 3
tile : 4
/****** UnPausing Ucdma ******/
no of cols :5
Loc addr :0
tile : 0
tile : 1
tile : 2
tile : 3
tile : 4
1/1 AIEML tests passed.
Tests Execution Completed


