LZP_SOURCES = lzp.c mpc.c lzp_core.c
LZP_HEADERS = prelude.h
LZP_OUTPUT = lzp.exe

build: $(LZP_HEADERS) $(LZP_SOURCES)
	xxd -i prelude.lzp > prelude.h
	gcc -O3 $(LZP_SOURCES) -o $(LZP_OUTPUT) -Wl,--stack,16777216

run: build
	@$(LZP_OUTPUT)

test: build
	@$(LZP_OUTPUT) -n ./tests/builtin.lzp
