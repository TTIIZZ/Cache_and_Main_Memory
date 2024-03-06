make: main1.c main2.c input.c 
	gcc -o main1 main1.c
	gcc -o main2 main2.c
	gcc -o input input.c

clean:
	rm main1 main2 input

