//
// Created by andre on 5/16/18.
//

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

int main(){
    int err;
    char * payload = malloc(10);
    payload = "hello";
    void * checkPayload = malloc(10);


    t_lista * head = iniLista();
    head = criaNovoNoLista(head,(void*)payload,&err);
    checkPayload = getItemLista(head);
    printf("%s\n",(char*)checkPayload);
}