#!/bin/bash

rm chunky.bib
set -e
pdflatex -halt-on-error chunky
bibtex chunky
pdflatex -halt-on-error chunky
pdflatex -halt-on-error chunky

