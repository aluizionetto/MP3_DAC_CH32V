/*
 * buffer_fifo.c
 *
 *  Created on: Dec 5, 2024
 *      Author: Aluizio d'Affonsêca Netto
 */

/* funções e estruturas para FIFO */

#include "buffer_fifo.h"

//inicializa buffer
void Init_FIFO(SBUFF *b, void *AR, int size_var ,int len) {
    b->ar = (unsigned char*)AR;
    b->len = len;
    b->bs_len = len*size_var; //taamnaho maximo em bytes
    b->size_v = size_var;
    b->in = 0;  //(in) posição do byte no array (ar)
    b->out = 0; //(out) posição do byte no array (ar)
    b->full = 0;
    b->count = 0;  //contagem de dados
}

//retorna numero de amotras que entraram no FIFO
int n_sample_in_FIFO(SBUFF *b) {
    if (b->full) return b->len;
    return b->count;
}

//retorna numero de amostras livres
int n_sample_out_FIFO(SBUFF *b) {
    if (b->full) return 0;
    return (b->len - b->count);
}

void put_sample_FIFO(SBUFF *b, void *s) {
	unsigned char *s_ = (unsigned char *)s;
    	if (!b->full) {
    		for(int k=0; k < b->size_v; k++) {
    			b->ar[b->in++]  = s_[k];
    			if (b->in == b->bs_len) b->in = 0;
    		}
    		b->count++;
    	}
    	if(b->count == b->len) b->full = 1;

}

void get_sample_FIFO(SBUFF *b, void *s) {
	unsigned char *s_ = (unsigned char *)s;
	if (b->count > 0) {
		for(int k = 0; k < b->size_v; k++) {
			s_[k] = b->ar[b->out++];
			if(b->out == b->bs_len) b->out = 0;
		}
    		b->full = 0;
    		b->count--;
    	}

}


