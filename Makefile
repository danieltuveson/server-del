CFLAGS = -O2 -g -Wall -Wextra
LDFLAGS = -ldel -levent -luuid -lsqlite3
objects = main.o globals.o delfuncs.o sql.o

server: $(objects)
	cc $(CFLAGS) $(objects) -o server $(LDFLAGS)

# Installs deps on Debian
deps:
	@git clone "git@github.com:danieltuveson/del.git"
	@$(MAKE) -C del
	@sudo $(MAKE) -C del install
	@cd ..
	@rm -rf del
	@sudo apt-get install libevent-dev
	@sudo apt-get install uuid-dev
	@sudo apt-get install sqlite3
	@sudo apt-get install libsqlite3-dev

clean:
	rm -f server *.o *.a
	rm -rf del

