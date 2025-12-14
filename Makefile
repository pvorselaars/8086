TESTS = $(basename $(wildcard tests/*.asm))

test: 8086
	@for t in ${TESTS}; do \
		nasm $$t.asm -o $$t.bin; \
		./8086 $$t.bin > $$t.test.s; \
		nasm $$t.test.s; \
		cmp -l $$t.test $$t.bin || echo $$t failed; \
	done

8086: main.c
	cc -Wall -o 8086 main.c

