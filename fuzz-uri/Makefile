CC      := clang
SHIM    := ../../fuzz-support
SRCS    := harness.c uri_parser_extracted.c $(SHIM)/php_stubs.c
CFLAGS  := -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all \
           -fno-omit-frame-pointer -g -O1 -Wall -I$(SHIM)
LDFLAGS := -L/tmp -L/usr/lib/x86_64-linux-gnu
RUN_ARGS := corpus/ -print_final_stats=1 -artifact_prefix=artifacts/

harness: $(SRCS)
	@mkdir -p artifacts
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)

run: harness
	./harness $(RUN_ARGS) -max_total_time=300

clean:
	rm -f harness
	rm -rf artifacts

.PHONY: run clean
