# Adding New System-C Tests

This document provides a step-by-step guide on how to add new SystemC tests to the project
and run and integrate them into the existing testing infrastructure as part of the AIEML Tests.


## 1. Writing the SystemC Test

1. **Create a New File**:

Create a new C file for your SystemC test in the `tests/stest` directory.
Name the file descriptively. For example, `xai_new_feature_test.c`.

2. **Write the Test**: Implement your SystemC test in the new file. Ensure that your test includes the necessary `#include` directives for SystemC and any other required headers.

  ```c
  int xai_new_feature_test(XAie_DevInst *DevInst)
  {
	//test body
  }
  ```

## 2. Tagging the test function to aiert\_tests.h

1. **Link the new Test**: Within the aiert\_tests.h, add the new test function pointer and the name string.

    ```c
    extern int xai_new_feature_test(XAie_DevInst *DevInst);

    int (*tests_aiert[])(XAie_DevInst *DevInst) =
    {
      test_lock,
      xai_new_feature_test
    }

    const char *test_names_aiert[] =
    {
      "test_lock"
      "xai_new_feature_test"
    };
    ```

## 3. Build and Run Locally

1. **Clone and Build aieml-tests with your modified aie-rt**:

```
  git clone https://gitenterprise.xilinx.com/ai-engine/aieml-tests.git
  mv <your aie-rt with stests> aieml-tests
  cd aieml-tests
  <source environment variables needed by aieml-tests>
  make JENKINS_TESTS=yes #this extra flag makes aieml-tests's Makefile also look into aie-rt/driver/tests/stest/ for system-c tests
  make run_sim
```

## 4. Build and Run through Testing Infrastructure

1. **Create a pull request**: Create a pull request from your aie-rt forked repo, and the tests will automatically be triggered.

2. **Status Check**: Your new system-c test will be run as part of the aieml-tests. Hence, you can check for the status of the aieml-tests under the "console output" for the jenkins job "aieml-tests".

## Conclusion

You have now successfully added a new SystemC test to the project. If you wish to integrate the new stest with the ai-engine/aie-rt repo, sending out the patch will automatically do that. Ensure that your test is thorough and covers all relevant scenarios to maintain a robust and reliable codebase.

If you encounter any issues or need further assistance, please contact the project maintainers or consult the project's contribution guidelines.
