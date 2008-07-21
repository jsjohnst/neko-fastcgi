all: build nekocgi

nekocgi: build/main.o build/request.o build/fcgi_reader.o build/utils.o
	$(LINK.o) -o $@ $^ -lneko -lfcgi

build/%.o: src/%.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

build/:
	mkdir -p build

clean:
	rm -rf build

realclean: clean
	rm -f nekocgi 
