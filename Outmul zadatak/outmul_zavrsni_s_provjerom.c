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
	double *c;
} thr_struct;
thr_struct *nova;

pthread_barrier_t barijera;

extern void dger_(int *M, int *N, double *ALPHA, double *X, int *INCX, double *Y, int *INCY, double *A, int *LDA);

void* thr_f( void* arg )
{
	int indeks 			= ( (thr_struct*) arg )->indeks;
	int pocetak 			= ( (thr_struct*) arg )->pocetak;
	int kraj 			= ( (thr_struct*) arg )->kraj;
	int broj_stupaca 			= ( (thr_struct*) arg )->broj_stupaca;

	//printf( "START: %d, SIZE=%d\n", start, size );

	double alf = 1;
	int move_a = 1, move_b = n, i, alfa=1;

	for(i=0; i<n; ++i)
    {
			//dger_( &m, &size, &alpha, &a(0,i), &move_a,&b(i,start), &move_b, &c(0,start), &m );
			dger_(&m, &broj_stupaca, &alf, A+(i*m), &alfa, B+(pocetak*n)+i, &n, C+(pocetak*m), &m);
    }
   // printf("Prije barijere %d\n", indeks);
    pthread_barrier_wait(&barijera);
   // printf("Poslije barijere %d\n", indeks);
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



int main(int argc, char **argv)
{
    if (argc != 8)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    char *ime_a, *ime_b, *ime_c;
    int i, j, k;
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
//    ispis_matrice(A, m, n);
//    ispis_matrice(B, n, p);

    clock_t trajanje;

    trajanje = clock();
    double C_normalno[m*p];
    for (i=0; i<p; ++i)
        for(j=0; j<m; ++j)
        {
            double suma=0;
            for (k=0; k<n; k++)
                suma+=A[j+k*m]*B[i*n+k];
            C_normalno[i*m+j]=suma;
    	}
    trajanje -= clock();
    printf("Obicno mnozenje treba %f sekundi\n", ((float)-trajanje)/CLOCKS_PER_SEC);
//    ispis_matrice(C_normalno, m, p);

    /*double B_tran[n*p];
    for(i=0; i<n; ++i)
        for(j=0; j<p; ++j)
            B_tran[i*n+j] = B[j*p+i];*/
    double CC[m*p];

    trajanje = clock();
    int alfa=1;
    double alf=1;
    for(j=0; j<n; ++j)  //dobijemo C matricu popunjemu umnoskom a i b matrice, samo je c transponirana
            dger_(&m, &p, &alf, A+(j*m), &alfa, B+(j), &n, CC, &m);
    trajanje -= clock();
    printf("Dger mnozenje bez paralelizacije treba   %f   sekundi\n", ((float)-trajanje)/CLOCKS_PER_SEC);
    ispis_matrice(CC, m,p);


    trajanje = clock();
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
        //printf("stvaramo %d dretvu\n", i);
        if(ostatak)
        {
        	ostatak--;
        	ekstra_stupac=1;
        }
        else
        	ekstra_stupac=0;
        //(ostatak--) ? ekstra_stupac = 1 : ekstra_stupac = 0;
        nova[i].c = (double*)malloc(m*(novi_stupci_b+ekstra_stupac)*sizeof(double));
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
		printf("stvorena %d dretva, ide od %d stupca, do %d stupca\n", i, nova[i].pocetak, nova[i].kraj);
    }
    thr_struct *ptr;
    if (pthread_join(thr_idx[0], (void*)&ptr))
    {
    	fprintf(stderr, "%s: error joining thread 0\n", argv[0]);
    	exit(EXIT_FAILURE);
    }

	trajanje -= clock();
    printf("Dger mnozenje s dretvama treba   %f   sekundi\n", ((float)-trajanje)/CLOCKS_PER_SEC);
    ispis_matrice(C, m,p);


    return 0;
}
