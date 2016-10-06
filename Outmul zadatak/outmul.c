#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>


pthread_mutex_t lock;
int m, n, p, broj_dretvi;
double *A, *B, *C;

typedef struct _thr_struct{
	int indeks, pocetak, kraj, broj_stupaca;
} thr_struct;
thr_struct *nova;

pthread_barrier_t barijera;

extern void dger_(int *M, int *N, double *ALPHA, double *X, int *INCX, double *Y, int *INCY, double *A, int *LDA);

void* thr_f( void* arg )
{
	int pocetak 		= ( (thr_struct*) arg )->pocetak;
	int broj_stupaca    = ( (thr_struct*) arg )->broj_stupaca;

	double alf = 1;
	int i, alfa=1;

	for(i=0; i<n; ++i)
        dger_(&m, &broj_stupaca, &alf, A+(i*m), &alfa, B+(pocetak*n)+i, &n, C+(pocetak*m), &m);

    pthread_barrier_wait(&barijera);
	return NULL;
}

void ispis_matrice(double *a, int x, int y)
{
    int i, j;
    for(i=0; i<x; ++i)
    {
    	for(j=0; j<y; ++j)
		printf("%lg ", a[j*x+i]);
        printf("\n");
    }
    printf("\n");
}

void ucitaj (int duljina, char *ime_datoteke, double *x)  //ucitavanje matrice
{
    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "rb");
    if(matrice_file == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", ime_datoteke);
    fread(x, sizeof(double), duljina, matrice_file);
    fclose(matrice_file);
}

void ispis_u_datoteku (int duljina, char *ime_datoteke, double *x)  //ucitavanje matrice
{
    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "wb");
    if(matrice_file == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", ime_datoteke);
    fwrite(x, sizeof(double), duljina, matrice_file);
    fclose(matrice_file);
}

int main(int argc, char **argv)
{
    if (argc != 8)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    char *ime_a, *ime_b, *ime_c;
    int i;
    broj_dretvi = atoi(argv[1]);
    m = atoi(argv[2]);
    n = atoi(argv[3]);
    p = atoi(argv[4]);
    ime_a = argv[5];
    ime_b = argv[6];
    ime_c = argv[7];

    A = (double*)malloc(m*n*sizeof(double));
    B = (double*)malloc(n*p*sizeof(double));
    C = (double*)malloc(m*p*sizeof(double));

    ucitaj(m*n, ime_a, A);
    ucitaj(n*p, ime_b, B);

    pthread_t *thr_idx;
    pthread_barrier_init(&barijera, NULL, broj_dretvi);
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL)
    {
		fprintf(stderr, "%s: memory allocation error\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int novi_stupci_b = p/broj_dretvi;
    int ostatak = p-broj_dretvi*novi_stupci_b;
    int ekstra_stupac, ukupno=0;
    nova = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct));
	for (i = 0; i < broj_dretvi; ++i)
	{
        if(ostatak)
        {
        	ostatak--;
        	ekstra_stupac=1;
        }
        else
        	ekstra_stupac=0;

        nova[i].broj_stupaca = novi_stupci_b+ekstra_stupac;
        nova[i].indeks = i;
        nova[i].pocetak = ukupno;
        nova[i].kraj = ukupno + novi_stupci_b+ekstra_stupac;
        ukupno = nova[i].kraj;
		if (pthread_create(&thr_idx[i], NULL, thr_f,(void*)&nova[i]))
        {
			printf( "%s: error creating thread %d\n", argv[0], i);
			exit(EXIT_FAILURE);
		}
    }

    thr_struct *ptr;
    if (pthread_join(thr_idx[0], (void*)&ptr))
    {
    	fprintf(stderr, "%s: error joining thread 0\n", argv[0]);
    	exit(EXIT_FAILURE);
    }

    ispis_u_datoteku(m*p, ime_c, C);

    return 0;
}
