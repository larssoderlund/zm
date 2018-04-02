# zm
Zero make - zero config buildsystem

zm is a zero make configuration build system. It adds functionality
that ideally should be included in a sane compiler.

zm is used as a replacement for gcc. For instance:

```
./zm -Wall -pedantic -g  -c link.c -o link.o
./zm -Wall -pedantic -g  -c zm.c    -o zm.o
./zm link.o zm.o -o zm $(pkg-config sqlite3 --libs)
```

Zm stores commandlines in a local sqlite3-database. When zm is called again
it checks the database and mtime of the c-files to determine which files
needs to be rebuilt.

Once the build database is built is only required to rebuild by issuing:

```
./zm
```
