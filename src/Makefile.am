bin_PROGRAMS = rvdb
rvdb_CFLAGS = $(DEPS_CFLAGS) --pedantic -Wall
rvdb_LDADD = $(DEPS_LIBS) -L/usr/include -lreadline
rvdb_SOURCES = \
    cli.c cli.h data.c data.h \
    debug.c debug.h file_io.c file_io.h \
    main.c serial.c serial.h types.h \
    util.c util.h pollLib.c pollLib.h
