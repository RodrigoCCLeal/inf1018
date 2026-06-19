#include <stdio.h>
#include <string.h>
#include "cria_func.h"

/* ---- funcoes "originais" usadas nos testes ---- */
int mult(int x, int y)            { return x * y; }
int identidade(int x)             { return x; }
int soma3(int a, int b, int c)    { return a + b + c; }

/* tipos de ponteiro para as novas funcoes geradas */
typedef int (*f1arg)(int);
typedef int (*f0arg)(void);
typedef int (*fprefixo)(void *candidata, size_t n);

char fixa[] = "quero saber se a outra string e um prefixo dessa";

static int ok = 0, falhas = 0;
static void checa(const char *nome, long obtido, long esperado) {
    if (obtido == esperado) {
        printf("  [OK]    %-40s = %ld\n", nome, obtido);
        ok++;
    } else {
        printf("  [FALHA] %-40s = %ld (esperado %ld)\n", nome, obtido, esperado);
        falhas++;
    }
}

int main(void) {
    unsigned char codigo[500];
    DescParam params[3];
    int i;

    /* ================================================================
       TESTE 1 - 1 parametro, PARAM puro (repasse)
       nova = identidade(x)   ->  nova(x) == x
       ================================================================ */
    params[0].tipo_val = INT_PAR; params[0].orig_val = PARAM;
    cria_func(identidade, params, 1, codigo);
    {
        f1arg nova = (f1arg) codigo;
        printf("Teste 1: 1 param PARAM (identidade)\n");
        checa("identidade(7)", nova(7), 7);
        checa("identidade(-5)", nova(-5), -5);
    }

    /* ================================================================
       TESTE 2 - 1 parametro, FIX (constante amarrada)
       nova = identidade(42)  ->  nova() == 42
       ================================================================ */
    params[0].tipo_val = INT_PAR; params[0].orig_val = FIX;
    params[0].valor.v_int = 42;
    cria_func(identidade, params, 1, codigo);
    {
        f0arg nova = (f0arg) codigo;
        printf("Teste 2: 1 param FIX (constante 42)\n");
        checa("identidade_fix()", nova(), 42);
    }

    /* ================================================================
       TESTE 3 - 1 parametro, IND (valor corrente de variavel)
       ================================================================ */
    {
        int v = 0;
        params[0].tipo_val = INT_PAR; params[0].orig_val = IND;
        params[0].valor.v_ptr = &v;
        cria_func(identidade, params, 1, codigo);
        f0arg nova = (f0arg) codigo;
        printf("Teste 3: 1 param IND (variavel)\n");
        v = 100; checa("identidade_ind() com v=100", nova(), 100);
        v = 777; checa("identidade_ind() com v=777", nova(), 777);
    }

    /* ================================================================
       TESTE 4 - mult com [PARAM, FIX=10]  (exemplo 1 do enunciado)
       nova(i) == i*10
       ================================================================ */
    params[0].tipo_val = INT_PAR; params[0].orig_val = PARAM;
    params[1].tipo_val = INT_PAR; params[1].orig_val = FIX;
    params[1].valor.v_int = 10;
    cria_func(mult, params, 2, codigo);
    {
        f1arg nova = (f1arg) codigo;
        printf("Teste 4: mult [PARAM, FIX=10]\n");
        for (i = 1; i <= 3; i++) {
            char nm[32]; sprintf(nm, "mult10(%d)", i);
            checa(nm, nova(i), i * 10);
        }
    }

    /* ================================================================
       TESTE 5 - mult com [IND=&i, FIX=10]  (exemplo 2 do enunciado)
       nova() == i*10   (i amarrado por endereco)
       ================================================================ */
    params[0].tipo_val = INT_PAR; params[0].orig_val = IND;
    params[0].valor.v_ptr = &i;
    params[1].tipo_val = INT_PAR; params[1].orig_val = FIX;
    params[1].valor.v_int = 10;
    cria_func(mult, params, 2, codigo);
    {
        f0arg nova = (f0arg) codigo;
        printf("Teste 5: mult [IND=&i, FIX=10]\n");
        for (i = 1; i <= 3; i++) {
            char nm[32]; sprintf(nm, "i=%d -> nova()", i);
            checa(nm, nova(), i * 10);
        }
    }

    /* ================================================================
       TESTE 6 - mult com [FIX=10, PARAM]  (ordem inversa: testa
       que NAO sobrescrevemos o PARAM ao amarrar o 1o argumento)
       nova(i) == 10*i
       ================================================================ */
    params[0].tipo_val = INT_PAR; params[0].orig_val = FIX;
    params[0].valor.v_int = 10;
    params[1].tipo_val = INT_PAR; params[1].orig_val = PARAM;
    cria_func(mult, params, 2, codigo);
    {
        f1arg nova = (f1arg) codigo;
        printf("Teste 6: mult [FIX=10, PARAM]\n");
        checa("nova(7)", nova(7), 70);
        checa("nova(9)", nova(9), 90);
    }

    /* ================================================================
       TESTE 7 - soma3 [FIX=1000, PARAM, PARAM]
       Caso critico: 2 PARAMs precisam ser deslocados de
       (rdi,rsi) para (rsi,rdx). Testa a copia de tras para frente.
       nova(a,b) == 1000 + a + b
       ================================================================ */
    params[0].tipo_val = INT_PAR; params[0].orig_val = FIX;
    params[0].valor.v_int = 1000;
    params[1].tipo_val = INT_PAR; params[1].orig_val = PARAM;
    params[2].tipo_val = INT_PAR; params[2].orig_val = PARAM;
    cria_func(soma3, params, 3, codigo);
    {
        int (*nova)(int, int) = (int(*)(int,int)) codigo;
        printf("Teste 7: soma3 [FIX=1000, PARAM, PARAM]\n");
        checa("nova(3,4)", nova(3, 4), 1007);
        checa("nova(20,5)", nova(20, 5), 1025);
    }

    /* ================================================================
       TESTE 8 - soma3 [PARAM, FIX=100, IND=&z]
       Mistura os 3 tipos de origem.
       nova(a) == a + 100 + z
       ================================================================ */
    {
        int z = 0;
        params[0].tipo_val = INT_PAR; params[0].orig_val = PARAM;
        params[1].tipo_val = INT_PAR; params[1].orig_val = FIX;
        params[1].valor.v_int = 100;
        params[2].tipo_val = INT_PAR; params[2].orig_val = IND;
        params[2].valor.v_ptr = &z;
        cria_func(soma3, params, 3, codigo);
        int (*nova)(int) = (int(*)(int)) codigo;
        printf("Teste 8: soma3 [PARAM, FIX=100, IND=&z]\n");
        z = 5;  checa("nova(1) z=5",  nova(1),  1 + 100 + 5);
        z = 50; checa("nova(8) z=50", nova(8),  8 + 100 + 50);
    }

    /* ================================================================
       TESTE 9 - memcmp [FIX=fixa(PTR), PARAM(PTR), PARAM(INT)]
       (exemplo "mesmo_prefixo" do enunciado)
       ================================================================ */
    params[0].tipo_val = PTR_PAR; params[0].orig_val = FIX;
    params[0].valor.v_ptr = fixa;
    params[1].tipo_val = PTR_PAR; params[1].orig_val = PARAM;
    params[2].tipo_val = INT_PAR; params[2].orig_val = PARAM;
    cria_func(memcmp, params, 3, codigo);
    {
        fprefixo mesmo_prefixo = (fprefixo) codigo;
        char s[] = "quero saber tudo";
        printf("Teste 9: memcmp [FIX=fixa, PARAM, PARAM]\n");
        /* "quero saber " (12 chars) e prefixo comum -> memcmp == 0 */
        checa("prefixo-12 igual? (0=sim)", mesmo_prefixo(s, 12), 0);
        /* string inteira nao e prefixo -> != 0 */
        checa("prefixo-16 diferente? (!=0)", mesmo_prefixo(s, strlen(s)) != 0, 1);
        printf("    '%s' tem mesmo prefixo-12 de '%s'? %s\n",
               s, fixa, mesmo_prefixo(s, 12) ? "NAO" : "SIM");
    }

    printf("\n==== RESUMO: %d OK, %d FALHAS ====\n", ok, falhas);
    return falhas ? 1 : 0;
}
