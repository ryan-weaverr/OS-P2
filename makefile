all: 
	gcc -g -Wall driver.c resource.c resource.h
	./a.out
clean: 
	$(RM) ./a.out resource.h.gch