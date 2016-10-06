#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>
#include <time.h>



void ucitaj (int duljina, char *ime_datoteke, char *x)  //ucitavanje matrice
{
    int i;
    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "rb");
    for (i=0; i<duljina; ++i)
        fscanf(matrice_file, "%c", &x[i]);
    fclose(matrice_file);
}

int main(int argc, char **argv)
{
    time_t timer, timer2;
    double seconds;
    time(&timer);

    if (argc != 5)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    char *ime_datoteke, *matrica;
    int m, n, i, broj_dretvi, duljina;

    broj_dretvi = atoi(argv[1]);
    m = atoi(argv[2]);
    n = atoi(argv[3]);
    ime_datoteke = argv[4];
    duljina = m*n;    //duljina je velicina matrice plus jos jedan stupac za oznaku novog retka

    matrica = (char*)malloc(duljina*sizeof(char));
    ucitaj(duljina, ime_datoteke, matrica);

    time(&timer2);
    seconds = difftime(timer2,timer);
    printf ("%lg seconds ", seconds);
    time(&timer);

    #pragma omp parallel num_threads(broj_dretvi)
    {
        #pragma omp for
        for (i=0; i<duljina; ++i)
        {
            if(matrica[i] == 'o')
            {
                #pragma omp critical
                {
                    printf("Kamp je na (%d,%d)\n", i/n, i%n);
                }
            }
        }
    }

    time(&timer2);
    seconds = difftime(timer2,timer);
    printf ("%lg seconds ", seconds);

    return 0;
}

