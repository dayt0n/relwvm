IOS_MIN_VERSION=6.0

ifeq (${TARGET},iphoneos-arm)
	CC=xcrun -sdk iphoneos clang -arch armv7 -miphoneos-version-min=${IOS_MIN_VERSION}
else
	CC=clang
endif

RAMDISK_SIZE=$(shell ./rdsz.sh)k


all: lwvmedit


# lwvmedit

lwvmedit: lwvmedit.c lwvmedit.h
	${CC} lwvmedit.c genguid.c -o lwvmedit

install: lwvmedit
	install -C lwvmedit /usr/bin


# ramdisk

ramdisk.dmg: ramdisk/init/init ramdisk/fs/usr/sbin/lwvmedit
	if [ -a ramdisk.dmg ] ; \
	then \
		rm ramdisk.dmg ; \
	fi;
	hdiutil create -srcfolder ramdisk/fs -volname Update ramdisk.dmg

ramdisk/fs/usr/sbin/lwvmedit:
	cp lwvmedit ramdisk/fs/usr/sbin/

ramdisk/init/init: ramdisk/init/init.c
	${CC} ramdisk/init/init.c -o ramdisk/init/init
	cp ramdisk/init/init ramdisk/fs/sbin/launchd


deviceinstall:
	@echo "Not implemented."
