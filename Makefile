all: predict predict8

predict: predict.c
	gcc -Wall -O3 predict.c -o predict -lm

predict8: predict8.c
	gcc -Wall -g predict8.c -o predict8 -lm
