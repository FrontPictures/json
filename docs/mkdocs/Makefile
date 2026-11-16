.PHONY: build serve install_venv

build:
	venv/bin/mkdocs build --strict

serve:
	venv/bin/mkdocs serve

install_venv:
	python3 -m venv venv
	venv/bin/python -m pip install -r requirements.txt
