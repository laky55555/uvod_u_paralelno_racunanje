#include <stdio.h>
#include <complex.h>
#include <stdlib.h>



int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "A.dat - binarna datoteka, sadrzi double complex, spremljena po retcima\n");
        fprintf(stderr, "int koji oznacava red matrica\n");
        return 0;
    }
    int broj_znakova, i, j, red_matrice;
    //double _Complex *a;
    double _Complex *a;

    red_matrice = atoi(argv[2]);
    broj_znakova = red_matrice*red_matrice;
    //a = (double complex*)malloc(broj_znakova*sizeof(double complex));
    a = (double _Complex *)malloc(broj_znakova*sizeof(double _Complex));

    FILE *fpb; // *fpn;
    fpb = fopen(argv[1], "wb");
    //fpn = fopen("B4.txt", "w");

    for (i=0; i<red_matrice; ++i)
    {
        for(j=0; j<red_matrice; j++)
        {
            //[i] = rand() %10 + rand() % 10 *_Complex_I;
            //a[i*red_matrice+j] = i*red_matrice+j;
            //a[i*red_matrice+j] = rand()%10 + rand()%10*I;
            if(i==j)
                a[i*red_matrice+j] = 1;
            else
                a[i*red_matrice+j] = 0;
            //fprintf(fpn, "(%lg,%lg)\n", creal(a[i]), cimag(a[i]) );
            //fprintf(fpn, "%lg ", a[i*red_matrice+j] );

        }
        //fprintf(fpn, "\n");
    }

    //fwrite(a, sizeof(double complex), broj_znakova, fpb);
    fwrite(a, sizeof(double _Complex), broj_znakova, fpb);

    //fclose(fpn);
    fclose(fpb);
    free(a);


    return 0;
}

