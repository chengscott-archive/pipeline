CC0 = g++ -std=c++11 -O3 -Wall
CC = g++ -std=c++11 -g -Wall
OBJ = main.o memory.o
archiTA = ../archiTA
goldensim = $(archiTA)/simulator/pipeline

all: pipeline

pipeline: ${OBJ}
	$(CC) -o pipeline $^

%.o: %.cpp
	$(CC) -c $^

.PNOHY: run
run: clean golden pipeline
	./pipeline
	diff snapshot.rpt diff/snapshot.rpt
	diff error_dump.rpt diff/error_dump.rpt

.PNOHY: golden
golden: dimage.bin iimage.bin
	./$(goldensim)
	mv *.rpt diff/

.PNOHY: test
test: clean pipeline
	make -f makefile.test

.PHONY: clean
clean:
	rm -f pipeline *.o *.rpt
