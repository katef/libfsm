#include <stdlib.h>
#include <stdint.h>

int harness_fuzzer_target(const uint8_t *data, size_t size);

/* define libfuzzer's point of entry */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    return harness_fuzzer_target(data, size);
}
