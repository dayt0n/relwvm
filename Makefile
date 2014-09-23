CC=clang

all: lwvmedit

lwvmedit: lwvmedit.c lwvmedit.h
	${CC} lwvmedit.c -o lwvmedit

install: lwvmedit
	install -C lwvmedit /usr/bin

deviceinstall:
	@echo "Not implemented."
