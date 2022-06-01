all: shell 
	
shell:  *.o
	gcc -o shell *.c 

clean:
	rm -rf *.o shell

