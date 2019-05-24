mousetoy: mousetoy.c
	gcc -o mousetoy mousetoy.c -lX11 -lXi -lm
run: mousetoy
	./mousetoy
