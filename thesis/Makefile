SRC = \
      *.tex \
      *.bib \
      figures/*.pdf \
      figures/*.pdf_tex

TARGET = praca

%.pdf: figures/%.dot
	dot -Tps $< -o figures/$*.eps
	epstopdf figures/$*.eps
	rm figures/$*.eps

$(TARGET): $(SRC)
	rubber -d $@

clean:
	rubber --clean -d $(TARGET)

.PHONE: \
	clean
