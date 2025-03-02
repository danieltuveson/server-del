CFLAGS = -O2 -g -Wall -Wextra

server: main.c
	cc $(CFLAGS) -o server main.c -ldel -levent

install:
	@git clone "git@github.com:danieltuveson/del.git"
	@$(MAKE) -C del
	@sudo $(MAKE) -C del install
	@cd ..
	@rm -rf del
	@sudo apt-get install libevent-dev

clean:
	rm -f server *.o *.a
	rm -rf del

