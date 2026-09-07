#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint64_t lisp_int;
typedef double lisp_double;
typedef const char* lisp_str;

lisp_int lrt_print_int(const lisp_int value) {
	printf("%llu\n", (unsigned long long) value);
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

lisp_int lrt_read_int() {
	lisp_int value;
	scanf("%llu", &value);
	return value;
}

lisp_double lrt_read_double() {
	lisp_double value;
	scanf("%lf", &value);
	return value;
}

lisp_str lrt_read_str() {
	static char buffer[1024];

	if (fgets(buffer, sizeof(buffer), stdin) == NULL)
		return NULL;

	char* newline = strchr(buffer, '\n');
	if (newline)
		*newline = '\0';

	return buffer;
}
