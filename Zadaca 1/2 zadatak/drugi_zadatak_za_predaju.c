#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


void ucitaj (int duljina, char *ime_datoteke, double *x);
void ispis_u_datoteku (int duljina, int iteracija, double *niz_za_ispis);

double *A, *norma, *x, *r, omega, *delta;
int stupac, redak, i, broj_dretvi, broj_iteracija, inkrement=1, konvergencija=0;
char trans='T';
double skalar_1=1, skalar_2=-1;
FILE *rjesenje_file;

// dgem y := alpha*A*x + beta*y trans je T ako ako je matrica po redcima, nama je po redcima
extern void dgemv_(char *trans, int *M, int *N, double *ALPHA, double *matrica, int *LDA, double *X, int *INCX, double *BETA, double *Y, int *INCY);
// ddot skalarni produkt dva vektora c := x*y
extern double ddot_(int *duljina, double *prvi_vektor, int *INCX, double *drugi_vektor, int *INCY);
// dnrm2 2 norma vektora
extern double dnrm2_(int *duljina, double *vektor, int *INC);
// daxpy b := alpha*a+b
extern void daxpy_(int *duljina, double *ALPHA, double *prvi_vektor, int *INCX, double *drugi_vektor, int *INCY);

typedef struct _thr_struct{
	int indeks, pocetak_po_red, kraj_po_red, pocetak_po_stup, kraj_po_stup;
} thr_struct;
thr_struct *nova;

pthread_barrier_t barijera;

void* thr_f( void* arg )
{
    int indeks       		        = ( (thr_struct*) arg )->indeks;

    int pocetak_po_red 		        = ( (thr_struct*) arg )->pocetak_po_red;
	int kraj_po_red                 = ( (thr_struct*) arg )->kraj_po_red;
	int duljina_po_red              = kraj_po_red - pocetak_po_red +1;

	int pocetak_po_stup 		    = ( (thr_struct*) arg )->pocetak_po_stup;
	int kraj_po_stup                = ( (thr_struct*) arg )->kraj_po_stup;
	int duljina_po_stup             = kraj_po_stup - pocetak_po_stup +1;

    int k;

	// racunanje norme, norma je ista u svakom koraku pa je izracunamo unaprijed
	for(k=pocetak_po_stup;k<=kraj_po_stup; ++k)
	{
        norma[k] = dnrm2_(&redak, A+k, &stupac);
        norma[k] *= norma[k];
    }

    dgemv_(&trans, &stupac, &duljina_po_red, &skalar_2, A+pocetak_po_red*stupac, &stupac, x, &inkrement, &skalar_1, r+pocetak_po_red, &inkrement);

    while(1)//treba izvrtiti broj_iteracija puta
    {
        pthread_barrier_wait(&barijera);
        for (k=pocetak_po_stup; k<=kraj_po_stup; ++k)
            delta[k] = omega * ddot_(&redak, r, &inkrement, A+k, &stupac) / norma[k];
        pthread_barrier_wait(&barijera);

        daxpy_(&duljina_po_stup, &skalar_1, delta+pocetak_po_stup, &inkrement, x+pocetak_po_stup, &inkrement);
        pthread_barrier_wait(&barijera);
        if(indeks==0)
            ispis_u_datoteku(stupac, ++konvergencija, x);

        pthread_barrier_wait(&barijera);
        if(konvergencija == broj_iteracija)
            return NULL;

        dgemv_(&trans, &stupac, &duljina_po_red, &skalar_2, A+pocetak_po_red*stupac, &stupac, delta, &inkrement, &skalar_1, r+pocetak_po_red, &inkrement);
        //pthread_barrier_wait(&barijera);

    }
	return NULL;
}


void ucitaj (int duljina, char *ime_datoteke, double *x)  //ucitavanje matrice iz datoteke
{
    int i;
    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "rb");
    if(matrice_file == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", ime_datoteke);
    fread(x, sizeof(double), duljina, matrice_file);
    fclose(matrice_file);
}

void ispis_u_datoteku (int duljina, int iteracija, double *niz_za_ispis)  //ispisivanje rjesenja
{
    fprintf(rjesenje_file, "%d ", iteracija);
    for(i=0; i<duljina; ++i)
        fprintf(rjesenje_file, "%lg ", niz_za_ispis[i]);
    fprintf(rjesenje_file, "\n");
}



int main(int argc, char **argv)
{

    if (argc != 10)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    char *datoteka_matrica, *datoteka_vektor, *datoteka_pocetak, *datoteka_rjesenje;

    broj_dretvi = atoi(argv[1]);
    redak = atoi(argv[2]);
    stupac = atoi(argv[3]);
    omega = atof(argv[4]);
    broj_iteracija = atoi(argv[5]);
    datoteka_matrica = argv[6];
    datoteka_vektor = argv[7];
    datoteka_pocetak = argv[8];
    datoteka_rjesenje = argv[9];

    A = (double*)malloc(redak*stupac*sizeof(double));   //A je ucitana po retcima
    x = (double*)malloc(stupac*sizeof(double));
    r = (double*)malloc(redak*sizeof(double));
    delta = (double*)malloc(stupac*sizeof(double));
    norma = (double*)malloc(stupac*sizeof(double));     //norma je norma vektora matrice A po stupcima

    ucitaj(redak*stupac, datoteka_matrica, A);
    ucitaj(stupac, datoteka_pocetak, x);
    ucitaj(redak, datoteka_vektor, r);

    rjesenje_file = fopen(datoteka_rjesenje, "w");
    if(rjesenje_file == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", datoteka_rjesenje);

    ispis_u_datoteku(stupac, 0, x);

    pthread_t *thr_idx;
    pthread_barrier_init(&barijera, NULL, broj_dretvi);
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL)
    {
		fprintf(stderr, "%s: memory allocation error (greska kod alokacije dretvi)\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    nova = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct));


	for (i = 0; i < broj_dretvi; ++i)
	{
        nova[i].indeks = i;
        nova[i].pocetak_po_stup = ((i*stupac)/broj_dretvi);
        nova[i].kraj_po_stup = (((i+1)*stupac)/broj_dretvi)-1;

        nova[i].pocetak_po_red = ((i*redak)/broj_dretvi);
        nova[i].kraj_po_red = (((i+1)*redak)/broj_dretvi)-1;

        if (pthread_create(&thr_idx[i], NULL, thr_f,(void*)&nova[i]))
        {
			printf( "%s: error creating thread %d\n", argv[0], i);
			exit(EXIT_FAILURE);
		}
    }


    if (pthread_join(thr_idx[0], NULL)) //spajanje glavne dretve s novo napravljenima koje rade algoritam
    {
    	fprintf(stderr, "%s: error joining thread 0\n", argv[0]);
    	exit(EXIT_FAILURE);
    }


    free(A);
    free(x);
    free(r);
    free(delta);
    free(norma);
    free(nova);
    free(thr_idx);
    fclose(rjesenje_file);
    if(pthread_barrier_destroy(&barijera))
    {
    	fprintf(stderr, "%s: error destroying barrier\n", argv[0]);
    	exit(EXIT_FAILURE);
    }

    return 0;
}
