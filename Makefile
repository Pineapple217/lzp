build:
	gcc lzp.c mpc.c -o lzp
run: 
	@make --no-print-directory build
	@./lzp