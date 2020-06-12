PAPER = chunky
TEX = $(wildcard *.tex)
BIB = ${PAPER}.bib
FIGS = $(wildcard figures/*.pdf figures/*.png graphs/*.pdf graphs/*.png)
OPTS = --halt-on-error

.PHONY: all clean

$(PAPER).pdf: $(TEX) $(BIB) $(FIGS)
	echo $(FIGS)
	pdflatex $(OPTS) $(PAPER)
	bibtex $(PAPER)
	pdflatex $(OPTS) $(PAPER)
	pdflatex $(OPTS) $(PAPER)

clean:
	rm -f *.aux *.bbl *.blg *.log *.out $(PAPER).pdf

