# Makefile
amafir: amafir2.c
	gcc amafir2.c  -lm -lfftw3 -O2 -march=native -o amafir2
