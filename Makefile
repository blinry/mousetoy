POINTER_IDS := $(shell xinput|grep "master pointer"|grep -o "id=[0-9]\+"|grep -o "[0-9]\+"|tr "\n" " ")

mousetoy: mousetoy.c
	gcc -o mousetoy mousetoy.c -lX11 -lXi -lm
run: mousetoy
	./mousetoy $(POINTER_IDS)
