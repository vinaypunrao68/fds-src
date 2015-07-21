mkfile_path	:= $(abspath $(lastword $(MAKEFILE_LIST)))
top_dir := $(patsubst %/,%,$(dir $(mkfile_path)))

pip_requests_version := 2.7.0
pip_tabulate_version := 0.7.5

all: test

test: deps
	@cd $(top_dir)/fdscli/test && python -m unittest discover

deps:
	@pip list | grep 'requests ($(pip_requests_version))' > /dev/null || sudo pip install --upgrade requests==$(pip_requests_version)
	@pip list | grep 'tabulate ($(pip_tabulate_version))' > /dev/null || sudo pip install --upgrade tabulate==$(pip_tabulate_version)

install: deps test
	@cd $(top_dir) && sudo pip install -I .

uninstall:
	@cd $(top_dir) && sudo pip uninstall -y fdscli
