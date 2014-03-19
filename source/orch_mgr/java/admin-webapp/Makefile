topdir            := ../../..

compile:
	npm install
	grunt build
	cp -rv dist/* $(main_dir_lib)/admin-webapp/

webapp_clean:
	grunt clean
	rm -rf $(main_dir_lib)/admin-webapp/

include $(topdir)/Makefile.incl
