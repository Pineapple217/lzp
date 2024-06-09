build:
	xxd -i prelude.lzp > prelude.h
	gcc lzp.c mpc.c -o lzp
run: 
	@make --no-print-directory build
	@./lzp
test:
	@make --no-print-directory build
	@./lzp -n ./tests/builtin.lzp