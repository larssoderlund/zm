CC=gcc
CFLAGS= -Wall -pedantic -g 


all: 
	$(CC) $(CFLAGS) -c link.c -o link.o
	$(CC) $(CFLAGS) -c zm.c    -o zm.o
	$(CC) link.o zm.o -o zm $$(pkg-config sqlite3 --libs)

.PHONY: clean
clean:
	rm -f *.o .zm.db zm

.PHONY: sql
sql:
	rm -f .zm.db
	echo "CREATE TABLE rules(cfile varchar(256) NOT NULL, ofile varchar(256) not null, args varchar(512) not null);" | sqlite3 .zm.db

.PHONY: lsql
lsql:
	echo "select * from rules;" | sqlite3 .zm.db

.PHONY: dist
dist: clean
	(cd .. && tar cvvf zm.tar zm/*.c zm/*.h zm/Makefile && gzip zm.tar)

.PHONY: tags
tags: clean
	ctags -R *.[ch]

.PHONY: test
test: clean all
	./zm -Wall -pedantic -g -c link.c -o link.o
	echo "select * from rules;" | sqlite3 .zm.db
	./zm -Wall -pedantic -g -c zm.c    -o zm.o	
	echo "select * from rules;" | sqlite3 .zm.db
	./zm -Wall -pedantic -g -c zm.c
	echo "select * from rules;" | sqlite3 .zm.db
	./zm -Wall -c zm.c 
	echo "select * from rules;" | sqlite3 .zm.db
	./zm -Wall -c broken.c 
	echo "select * from rules;" | sqlite3 .zm.db
