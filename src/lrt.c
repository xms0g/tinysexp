#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

typedef uint64_t lisp_value;

lisp_value lrt_print_int(lisp_value value) {
	printf("%" PRIu64 "\n", (unsigned long long) value);
	return value;
}
