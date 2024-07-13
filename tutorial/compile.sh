#!/usr/bin/env sh
pdflatex -synctex=1 tut.tex
bibtex tut
pdflatex -synctex=1 tut.tex
pdflatex -synctex=1 tut.tex
