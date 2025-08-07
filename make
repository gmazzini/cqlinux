all:
	gcc -o cqrun cqrun.c -lX11 -lXtst -lm

clean:
	rm -f cqrun
