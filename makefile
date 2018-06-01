ALL: client server

CC = gcc         
CFLAGS = 
OBJ1 = app_teste.c
OBJ2 = clipboard.c
OBJ3 = socket_lib.c
OBJ4 = list.c
OBJ5 = library.c
OBJ6 = comunication.c
OBJ7 = functionality.c
EXEC1 = client
EXEC2 = server

$(EXEC1): $(OBJ1) $(OBJ3) $(OBJ5) 
		$(CC) $(OBJ1) $(OBJ3) $(OBJ5) $(CFLAGS) -o $(EXEC1) 
		
		
$(EXEC2): $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ6) $(OBJ7)  
		$(CC) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ6) $(OBJ7) $(CFLAGS) -pthread -o $(EXEC2) 
