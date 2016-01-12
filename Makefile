all: predict predict8 predict16

predict: predict.c
	gcc -Wall -O3 predict.c -o predict -lm

predict8: predict8.c
	gcc -Wall -g predict8.c -o predict8 -lm

predict16: predict16.c
	gcc -Wall -g predict16.c -o predict16 -lm
