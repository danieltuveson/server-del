CFLAGS = -O2 -g -Wall -Wextra
LDFLAGS = -ldel -levent -luuid -lsqlite3 -ldsalloc
objects = main.o globals.o route.o html.o delfuncs.o sql.o # rtable.o

server: r.gen.c $(objects) 
	cc $(CFLAGS) $(objects) -o server $(LDFLAGS)

r.gen.c: rtable.gperf
	@gperf rtable.gperf > r.gen.c

# Installs deps on Debian
deps:
	@git clone "git@github.com:danieltuveson/del.git"
	@$(MAKE) -C del
	@sudo $(MAKE) -C del install
	@cd ..
	@rm -rf del
	@sudo apt-get install libevent-dev
	@sudo apt-get install uuid-dev
	@sudo apt-get install libsqlite3-dev

# Useful but not required to build
dev-deps:
	@sudo apt-get install sqlite3
	@sudo apt-get install gperf

clean:
	rm -f server *.o *.a
	rm -rf del
	rm -f *.gen.c

nuke:
	rm data/*
	rm -rf projects/*
