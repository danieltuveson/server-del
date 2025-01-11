CFLAGS = -O2 -g -Wall -Wextra

server: main.c
	cc $(CFLAGS) -o server main.c -ldel -levent

install:
	@git clone "git@github.com:danieltuveson/del.git"
	@$(MAKE) -C del
	@sudo cp del/del.h /usr/local/include
	@sudo cp del/libdel.a /usr/local/lib
	@rm -rf del
	@sudo apt-get install libevent-dev

clean:
	rm -f server *.o *.a
	rm -rf del

