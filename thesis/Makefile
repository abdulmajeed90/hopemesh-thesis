SRC = \
      *.tex \
      *.bib \
      figures/*.pdf \
      figures/*.pdf_tex

dot_%.pdf: figures/%.dot
	dot -Tps $< -o figures/$*.eps
	epstopdf figures/$*.eps
	rm figures/$*.eps

neato_%.pdf: figures/%.dot
	neato -Tps $< -o figures/$*.eps
	epstopdf figures/$*.eps
	rm figures/$*.eps

presentation: $(SRC)
	rubber -d $@

praca-oneside: $(SRC)
	echo '\documentclass[12pt, oneside]{_praca_dyplomowa}' >class.tex
	rubber -d praca
	mv praca.pdf praca-oneside.pdf

praca-twoside: $(SRC)
	echo '\documentclass[12pt, twoside]{_praca_dyplomowa}' >class.tex
	rubber -d praca
	mv praca.pdf praca-twoside.pdf

all: presentation praca-oneside praca-twoside

clean:
	rubber --clean -d praca
	rubber --clean -d presentation

.PHONY: \
	clean
