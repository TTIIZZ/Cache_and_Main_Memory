MAIN=main

$(MAIN): $(MAIN).o
	gcc -o $(MAIN) $(MAIN).o

$(MAIN).o: $(MAIN).c
	gcc -c $(MAIN).c

clean:
	rm $(MAIN) $(MAIN).o

