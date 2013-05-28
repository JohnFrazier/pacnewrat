# pacnewrat - pacman config manager

OUT = pacnewrat
VERSION = $(shell git describe)

SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}
DISTFILES = Makefile pacnewrat.c

PREFIX ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man

CPPFLAGS := -DPACNEWRAT_VERSION=\"${VERSION}\" ${CPPFLAGS}
CFLAGS := -std=c99 -g -pedantic -Wall -Wextra ${CFLAGS} -g
LDFLAGS := -lcurl -lalpm ${LDFLAGS}

all: ${OUT} doc

${OUT}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

doc: pacnewrat.1
pacnewrat.1: README.pod
	pod2man --section=1 --center="Pacnewrat Manual" --name="PACNEWRAT" --release="pacnewrat ${VERSION}" $< > $@

strip: ${OUT}
	strip --strip-all ${OUT}

install: pacnewrat pacnewrat.1
	install -D -m755 pacnewrat ${DESTDIR}${PREFIX}/bin/pacnewrat
	install -D -m644 pacnewrat.1 ${DESTDIR}${MANPREFIX}/man1/pacnewrat.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	rm -f ${DESTDIR}${PREFIX}/bin/pacnewrat
	@echo removing man page from ${DESTDIR}${MANPREFIX}/man1/pacnewrat.1
	rm -f ${DESTDIR}/${MANPREFIX}/man1/pacnewrat.1

dist: clean
	mkdir pacnewrat-${VERSION}
	cp ${DISTFILES} pacnewrat-${VERSION}
	sed "s/\(^VERSION *\)= .*/\1= ${VERSION}/" Makefile > pacnewrat-${VERSION}/Makefile
	tar czf pacnewrat-${VERSION}.tar.gz pacnewrat-${VERSION}
	rm -rf pacnewrat-${VERSION}

distcheck: dist
	tar xf pacnewrat-${VERSION}.tar.gz
	${MAKE} -C pacnewrat-${VERSION}
	rm -rf pacnewrat-${VERSION}

clean:
	${RM} ${OUT} ${OBJ} pacnewrat.1

.PHONY: clean dist doc install uninstall
