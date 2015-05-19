# Workflows:
# 
# From scratch:
# 1.) Download latest gcovr tarball to this directory. Set GCOVR_TARBALL to the downloaded
#     filename.
# 2.) Set GCOVR_CONTENT to the name of the directory that GCOVR_TARBALL unpacks to.
# 3.) make $(GCOVR_UPDATED)/scripts/gcovr (replace $(GCOVR_UPDATED) with what it would evaluate to
#     if the make was run.
# 4.) Make changes in $(GCOVR_UPDATED).
# 5.) make gcovr-fds.patch.
# 6.) Add patch to source control.
# 7.) make $(GCOVR_ARTIFACT)/scripts/gcovr for the actual file to use.
#
# Modifying an existing patch:
# 1.) make $(GCOVR_ARTIFACT)/scripts/gcovr
# 2.) edit
# 3.) rm -rf $(GCOVR_UPDATED)
# 4.) cp -a $(GCOVR_ARTIFACT) $(GCOVR_UPDATED)
# 5.) make gcovr-fds.patch.

GCOVR_TARBALL  = gcovr-3.2.tar.gz
GCOVR_CONTENT  = gcovr-3.2
GCOVR_UPDATED  = $(GCOVR_CONTENT).new
GCOVR_ARTIFACT = $(GCOVR_CONTENT).build
GCOVR_PATCH    = gcovr-fds.patch

.PHONY: clean

clean:
	rm -rf "$(GCOVR_CONTENT)"
	rm -rf "$(GCOVR_UPDATED)"
	rm -rf "$(GCOVR_ARTIFACT)"

$(GCOVR_CONTENT)/scripts/gcovr: $(GCOVR_TARBALL)
	tar -xf "$(GCOVR_TARBALL)"

$(GCOVR_UPDATED)/scripts/gcovr: $(GCOVR_CONTENT)/scripts/gcovr
	cp -a "$(GCOVR_CONTENT)" "$(GCOVR_UPDATED)"

gcovr-fds.patch: $(GCOVR_CONTENT)/scripts/gcovr $(GCOVR_UPDATED)/scripts/gcovr
	# Diff return code of 1 means differences were found.
	diff -ur $(GCOVR_CONTENT) $(GCOVR_UPDATED) > gcovr-fds.patch; [ $$? -eq 1 ]

$(GCOVR_ARTIFACT)/scripts/gcovr: $(GCOVR_CONTENT)/scripts/gcovr gcovr-fds.patch
	rm -rf "$(GCOVR_ARTIFACT)"
	cp -a "$(GCOVR_CONTENT)" "$(GCOVR_ARTIFACT)"
	cd "$(GCOVR_ARTIFACT)"; patch -p1 < ../gcovr-fds.patch
