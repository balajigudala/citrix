This folder contains jenkins scripts.

e.g. Open the jenkins server and navigate to the jenkins job configuration page 
http://xsjsswsip50:8080/job/AIEML_Tests_From_PR/ 

The "Build Steps" stage would invoke this file like the following 

bash .jenkins/run_aieml_tests_with_stable_aie_rt.sh  

Debug the above shell file to resolve jenkins issues!
