build:
	xxd -i prelude.lzp > prelude.h
	gcc lzp.c mpc.c -o lzp
run: 
	@make --no-print-directory build
	@./lzp