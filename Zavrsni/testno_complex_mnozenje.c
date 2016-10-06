#include <stdio.h>
#include <complex.h>
#include <math.h>
#include <stdlib.h>


extern void zgemm_(char *TRANSA, char *TRANSB, int *M, int *N, int *K, double _Complex *ALPHA, double _Complex *A, int *LDA, double _Complex *B, int *LDB, double _Complex *BETA, double _Complex *C, int *LDC);


void ispis_matrice(double _Complex *a, int x, int y, char* ime)  //ispis matrice (za provjeru)
{
    printf("%s u transponiranom obliku:\n", ime);
    int k,l;
    for(k=0; k<x; ++k)
    {
    	for(l=0; l<y; ++l)
            printf("(%lg,%lg) ", creal(a[l*y+k]), cimag(a[l*y+k]) );
        printf("\n");
    }

    printf("\n");
}

void ucitaj(double _Complex *a, int red_matrice, char *A)
{
    printf("Ucitavanje matrica %s i %s\n", A);
    FILE *matrica_A;

    matrica_A = fopen(A, "rb");

    if(matrica_A == NULL)
    	printf("Greska pri otvaranju datoteke\n");

    fread(a, sizeof(double _Complex), red_matrice*red_matrice, matrica_A);

    fclose(matrica_A);
}

int main(int argc, char **argv)
{

    double _Complex *A, *B, *C, alfa=1;
    int red_matrice;
    char trans = 'T';

    if (argc != 4)
    {
        fprintf(stderr, "usage: %s A.dat, B.dat broj redaka \n", argv[0]);
        fprintf(stderr, "A.dat - binarna datoteka, sadrzi double complex, spremljena po retcima\n");
        fprintf(stderr, "B.dat - binarna datoteka, sadrzi double complex, spremljena po retcima\n");
        //fprintf(stderr, "C.dat - binarna datoteka, sadrzi double complex, spremljena po retcima\n");
        fprintf(stderr, "int koji oznacava red matrica\n");

    }

    red_matrice = atoi(argv[3]);
    printf("Red matrice je %d\n", red_matrice);


    printf("Prije alokacije\n");
    //A = (double *) calloc( red_matrice*red_matrice, sizeof(double));
    A = (double _Complex *) malloc(red_matrice*red_matrice*sizeof(double _Complex));
    B = (double _Complex *) malloc(red_matrice*red_matrice*sizeof(double _Complex));
    C = (double _Complex *) malloc(red_matrice*red_matrice*sizeof(double _Complex));
    //B = (double *) calloc( red_matrice*red_matrice, sizeof(double));
    //C = (double *) calloc( red_matrice*red_matrice, sizeof(double));

    printf("Poslije alokacije\n");
    if(A == NULL || B == NULL || C == NULL)
        printf("Greska kod alokacije matrica\n");

    ucitaj(A, red_matrice, argv[1]);
    ucitaj(B, red_matrice, argv[2]);

    zgemm_(&trans, &trans, &red_matrice, &red_matrice, &red_matrice, &alfa, A, &red_matrice, B, &red_matrice, &alfa, C, &red_matrice);



    //ispis_matrice(A, red_matrice, red_matrice, "Matrica A");
    //ispis_matrice(B, red_matrice, red_matrice, "Matrica B");
    ispis_matrice(C, red_matrice, red_matrice, "Matrica C");

    free(A);
    free(B);
    free(C);

    return 0;
}

