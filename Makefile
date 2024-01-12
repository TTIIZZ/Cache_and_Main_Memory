make: main.c input.c 
	gcc -o main main.c
	gcc -o input input.c

clean:
	rm main input

