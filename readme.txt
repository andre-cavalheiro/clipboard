List of stuff to do:


>Acabar o wait()
>new_data está em todo o lado mas nao está a ser usado em lado nenhum porque a cond.wait desbloqueia logo com o pthread_signal...
>FIXME
>When parent dies clipboard also dies why?
>Find if remote comunication breaks
> app_test sometimes prints trash. I dont think it's a clipboard problem but an app_test problem by reusing the same pointers over and over
>Valgrind
>makefile


Compiling

clipboard:

gcc -Wall -o socketlib.o -c socket_lib.c && gcc -c list.c -o lista.o && gcc -Wall -c -o comunication.o comunication.c && gcc -Wall -c -o functionality.o functionality.c  && gcc -Wall -pthread -o clipboard.o clipboard.c socketlib.o lista.o comunication.o functionality.o

client:

gcc -Wall -o lib.o -c library.c  && gcc -o app_test.o app_teste.c socketlib.o lib.o


Correr:

./clipboard.o	    (isto vai fazer printf de um porto e do nome de um socket CLIP...
./clipboard.o -c 127.0.0.1 <porto>

./app_test.o <CLIP...>

(o nome do socket CLIP... é baseado no PID e serve apenas para podermos correr mts clipboards na mm pasta)


