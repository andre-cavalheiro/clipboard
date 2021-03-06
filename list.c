/******************************************************************************
 *
 * File Name: lista.c
 *	      (c) 2009 AED
 * Authors:    AED Team
 * Last modified: ACR 2009-03-23
 * Revision:  v2.0
 *
 * COMMENTS
 *		implements functions for type t_listaPalavra
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "list.h"


/* Linked list  */
struct _t_lista {
  void *this;
  struct _t_lista *prox;
};


/******************************************************************************
 * iniLista ()
 *
 * Arguments: none
 * Returns: t_lista *
 * Side-Effects: list is initialized
 *
 * Description: initializes list
 *
 *****************************************************************************/

t_lista  *iniLista(void) {

  return NULL;
}


/******************************************************************************
 * criaNovoNoLista ()
 *
 * Arguments: nome - Item to save in list node
 * Returns: t_lista  *
 * Side-Effects: none
 *
 * Description: creates and returns a new node that can later be added to the
 *              list
 *
 *****************************************************************************/

t_lista  *criaNovoNoLista (t_lista* lp, void *this, int *err) {
  t_lista *novoNo;

  novoNo = (t_lista*) malloc(sizeof(t_lista));
  if(novoNo!=NULL) {

    novoNo->this = this;
    novoNo->prox = lp;
    lp = novoNo;
    *err = 0;
  } else {
    *err = 1;
  }
  return lp;
}


/******************************************************************************
 * getItemLista ()
 *
 * Arguments: this - pointer to element
 * Returns: Item
 * Side-Effects: none
 *
 * Description: returns an Item from the list
 *
 *****************************************************************************/

void *getItemLista (t_lista *p) {
  if(p==NULL){
      return NULL;
  }
  return p -> this;
}


/******************************************************************************
 * getProxElementoLista ()
 *
 * Arguments: this - pointer to element
 * Returns: pointer to next element in list
 * Side-Effects: none
 *
 * Description: returns a pointer to an element of the list
 *
 *****************************************************************************/

t_lista *getProxElementoLista(t_lista *p) {
  return p -> prox;
}


/******************************************************************************
 * numItensNaLista ()
 *
 * Arguments: lp - pointer to list
 * Returns:  count of the number of items in list
 * Side-Effects: none
 *
 * Description: returns the number of items (nodes) in the list
 *
 *****************************************************************************/

int numItensNaLista(t_lista *lp) {
  t_lista *aux;  /* auxiliar pointers to travel through the list */
  int conta = 0;
  aux = lp;

  for(aux = lp; aux != NULL; aux = aux -> prox)
    conta++;

  return conta;
}

/******************************************************************************
 * libertaLista ()
 *
 * Arguments: lp - pointer to list
 * Returns:  (void)
 * Side-Effects: frees space occupied by list items
 *
 * Description: free list
 *
 *****************************************************************************/

void libertaLista(t_lista *lp, void freeItem(void *)) {
  t_lista *aux, *newhead;  /* auxiliar pointers to travel through the list */

  for(aux = lp; aux != NULL; aux = newhead) {
    newhead = aux->prox;
    freeItem(aux->this);
    free(aux);
  }

  return;
}


/******************************************************************************
 * libertaLista ()
 *
 * Arguments: prev - pointer to previous node
 *            cur - current node
 * Returns:  (void)
 * Side-Effects: frees 
 *
 * Description: free current node
 *
 *****************************************************************************/
t_lista *free_node(t_lista * head,t_lista *cur,void freeItem(void *)) {
  t_lista *aux = head;

  if (head == cur) {
    head = head->prox;
    freeItem(cur);
  } else {
    while (getProxElementoLista(aux) != cur) {
      aux = getProxElementoLista(aux);
    }
    aux->prox = cur->prox;
    cur->prox = NULL;
    freeItem(cur);
  }
  return head;
}

/**
 *
 * @param head
 * @return
 */
t_lista * getLastNode(t_lista * head){
  t_lista * aux = head;
  while(getProxElementoLista(aux) != NULL){
    aux = getProxElementoLista(aux);
  }
  return aux;
}
