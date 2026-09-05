#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

typedef uint64_t lisp_int;
typedef double lisp_double;
typedef const char* lisp_str;

lisp_int lrt_print_int(lisp_int value) {
	printf("%" PRIu64 "\n", (unsigned long long) value);
	return value;
}

lisp_double lrt_print_double(const lisp_double value) {
	printf("%f\n", (double) value);
	return value;
}

lisp_str lrt_print_str(lisp_str value) {
	printf("%s\n", value);
	return value;
}
