/* Aluno1 Nome  Matricula  Turma */
/* Aluno2 Nome  Matricula  Turma */

#include <string.h>
#include "cria_func.h"

/*
 *
 * Como foram pedidos no maximo 3 parametros, so precisamos de
 *   posicao 0 -> %rdi
 *   posicao 1 -> %rsi
 *   posicao 2 -> %rdx
 *
 * Codigo do registrador no campo reg/rm do byte ModRM:
 *   rax=0 rcx=1 rdx=2 rbx=3 rsp=4 rbp=5 rsi=6 rdi=7
 */
static const unsigned char reg[3] = { 7, 6, 2 };  /* rdi, rsi, rdx */

/* grava um inteiro de 32 bits em little-endian e devolve a nova posicao */
static int emite32(unsigned char *c, int p, int valor) {
    unsigned int v = (unsigned int) valor;
    c[p++] = (unsigned char)( v        & 0xFF);
    c[p++] = (unsigned char)((v >>  8) & 0xFF);
    c[p++] = (unsigned char)((v >> 16) & 0xFF);
    c[p++] = (unsigned char)((v >> 24) & 0xFF);
    return p;
}

/* grava um valor de 64 bits (endereco) em little-endian */
static int emite64(unsigned char *c, int p, unsigned long v) {
    int i;
    for (i = 0; i < 8; i++)
        c[p++] = (unsigned char)((v >> (8 * i)) & 0xFF);
    return p;
}

void cria_func (void* f, DescParam params[], int n, unsigned char codigo[]) {
    int p = 0;   /* posicao corrente de escrita no vetor codigo */
    int i;

    /* ---------- PROLOGO ----------
       push %rbp ; mov %rsp,%rbp
       Realinha a pilha em multiplo de 16 (necessario para poder
       chamar funcoes de biblioteca a partir do codigo gerado). */
    codigo[p++] = 0x55;                 /* push %rbp       */
    codigo[p++] = 0x48;                 /* mov  %rsp,%rbp  */
    codigo[p++] = 0x89;
    codigo[p++] = 0xE5;

    /* ---------- PREPARACAO DOS ARGUMENTOS ----------
       Percorremos os parametros do ULTIMO para o PRIMEIRO.
       Isso garante que nunca sobrescrevemos um registrador de
       argumento recebido (PARAM) antes de copiar seu valor para
       o destino: a origem de um PARAM na posicao i e sempre um
       registrador de indice <= i, entao escrevendo de tras para
       frente o destino nunca e fonte de um parametro ainda nao
       processado. */
    for (i = n - 1; i >= 0; i--) {
        unsigned char rd = reg[i];      /* registrador destino (posicao i) */

        if (params[i].orig_val == PARAM) {
            /* j = quantos PARAM existem antes da posicao i;
               o valor chega no j-esimo registrador de argumento */
            int j = 0, k;
            for (k = 0; k < i; k++)
                if (params[k].orig_val == PARAM) j++;
            unsigned char rs = reg[j];  /* registrador de origem */

            /* mov %rs, %rd  ->  48 89 (C0 | rs<<3 | rd)  (move 64 bits) */
            codigo[p++] = 0x48;
            codigo[p++] = 0x89;
            codigo[p++] = (unsigned char)(0xC0 | (rs << 3) | rd);
        }
        else if (params[i].orig_val == FIX) {
            if (params[i].tipo_val == INT_PAR) {
                /* mov $imm32, %e(rd)  ->  (B8+rd) imm32   (zera bits altos) */
                codigo[p++] = (unsigned char)(0xB8 + rd);
                p = emite32(codigo, p, params[i].valor.v_int);
            } else {
                /* movabs $imm64, %r(rd)  ->  48 (B8+rd) imm64 */
                codigo[p++] = 0x48;
                codigo[p++] = (unsigned char)(0xB8 + rd);
                p = emite64(codigo, p, (unsigned long) params[i].valor.v_ptr);
            }
        }
        else { /* IND: passar o valor corrente da variavel apontada */
            /* movabs $endereco, %r11  ->  49 BB imm64 */
            codigo[p++] = 0x49;
            codigo[p++] = 0xBB;
            p = emite64(codigo, p, (unsigned long) params[i].valor.v_ptr);

            if (params[i].tipo_val == INT_PAR) {
                /* mov (%r11), %e(rd)  ->  41 8B (rd<<3 | 011) */
                codigo[p++] = 0x41;
                codigo[p++] = 0x8B;
                codigo[p++] = (unsigned char)((rd << 3) | 0x03);
            } else {
                /* mov (%r11), %r(rd)  ->  49 8B (rd<<3 | 011) */
                codigo[p++] = 0x49;
                codigo[p++] = 0x8B;
                codigo[p++] = (unsigned char)((rd << 3) | 0x03);
            }
        }
    }

    /* ---------- CHAMADA DA FUNCAO ORIGINAL ----------
       movabs $f, %rax ; call *%rax  (call indireto) */
    codigo[p++] = 0x48;                 /* movabs $f,%rax */
    codigo[p++] = 0xB8;
    p = emite64(codigo, p, (unsigned long) f);
    codigo[p++] = 0xFF;                 /* call *%rax     */
    codigo[p++] = 0xD0;

    /* ---------- EPILOGO ----------
       leave (desfaz o registro de ativacao) ; ret
       O valor de retorno permanece em %rax/%eax, intacto. */
    codigo[p++] = 0xC9;                 /* leave */
    codigo[p++] = 0xC3;                 /* ret   */
}
