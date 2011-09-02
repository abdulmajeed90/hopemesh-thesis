SRC = \
      *.tex \
      *.bib \
      figures/*.pdf_tex

TARGET = praca

$(TARGET): $(SRC)
	rubber -d $@

clean:
	rubber --clean -d $(TARGET)

.PHONE: \
	clean
