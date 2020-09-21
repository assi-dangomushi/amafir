# Makefile
amafir: amafir.c
	gcc amafir.c  -lm -lfftw3 -O2 -march=native -o amafir
