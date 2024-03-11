if {$argc >= 4} {
	set xsa "[lindex $argv 0]"
	set device "[lindex $argv 1]"
	set gen "[lindex $argv 2]"
	set drv "[lindex $argv 3]"
} else {
	puts "Error:Invalid arguments"
	puts "Enter:xsct aieml_tests.tcl <XSA location> <DEVICE> <HW GEN> <AIE DRIVER PATH>"
	exit 2
}

set xsaName [file rootname [file tail $xsa]]
set opts "-DDEVICE=$device -DAIE_GEN=$gen"

setws build_baremetal
repo -set ""
repo -scan
repo -apps
app create -name "aieml_vnc_tests" -hw "$xsa" -proc "psv_cortexa72_0" -os "standalone" -template "Empty Application"
importsource -name "aieml_vnc_tests" -path ./src -linker-script
bsp setlib -name xilpm
app config -name "aieml_vnc_tests" -add compiler-misc $opts
app build -name "aieml_vnc_tests"

set bspDir "[pwd]/build_baremetal/$xsaName/psv_cortexa72_0/standalone_domain/bsp"
set inc "$bspDir/psv_cortexa72_0/include"
set lib "$bspDir/psv_cortexa72_0/lib"
set libSrc "$bspDir/psv_cortexa72_0/libsrc"
set currDrv [glob -nocomplain -directory $libSrc -type d aienginev2_v*]

if {$currDrv eq ""} {
	set currDrv "$libSrc/aienginev2_v9_9"
}

#Replace library with local xaie lib
file delete -force $currDrv
file mkdir $currDrv
file copy -force $drv $currDrv

#Rebuild xaie
file mkdir "$inc/xaiengine"
exec -ignorestderr make -C "$currDrv/src" clean
exec -ignorestderr make -C $bspDir all

#Copy build artificats to export directory
set exportInc "[pwd]/build_baremetal/$xsaName/export/$xsaName/sw/$xsaName/standalone_domain/bspinclude"
set exportLib "[pwd]/build_baremetal/$xsaName/export/$xsaName/sw/$xsaName/standalone_domain/bsplib"

file delete -force "$exportLib/lib"
file delete -force "$exportInc/include"
file copy -force $inc $exportInc
file copy -force $lib $exportLib

#Rebuild application
set appSrc "[pwd]/build_baremetal/aieml_vnc_tests/Debug"
exec -ignorestderr make -C $appSrc clean all
