SRC = \
      *.tex \
      *.bib \
      figures/*.pdf \
      figures/*.pdf_tex

TARGET = praca

dot_%.pdf: figures/%.dot
	dot -Tps $< -o figures/$*.eps
	epstopdf figures/$*.eps
	rm figures/$*.eps

neato_%.pdf: figures/%.dot
	neato -Tps $< -o figures/$*.eps
	epstopdf figures/$*.eps
	rm figures/$*.eps

$(TARGET): $(SRC)
	rubber -d $@

clean:
	rubber --clean -d $(TARGET)

.PHONE: \
	clean
