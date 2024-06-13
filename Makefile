build:
	xxd -i prelude.lzp > prelude.h
	gcc -O3 lzp.c mpc.c -o lzp -Wl,--stack,16777216
run: 
	@make --no-print-directory build
	@./lzp
test:
	@make --no-print-directory build
	@./lzp -n ./tests/builtin.lzp