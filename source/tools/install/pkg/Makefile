# Makefile to create all our packages for install

all: package

.PHONY: package

clean:
	@sudo rm -f *.deb
	@sudo rm -rf tmppkg-*

package: clean
	@sudo fdspkgcreate -r