#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

char *matrica;
int m,n,brojac=0;

typedef struct _thr_struct{
	int indeks, pocetak_po_red, kraj_po_red;
} thr_struct;
thr_struct *nova;

pthread_mutex_t lock;
pthread_barrier_t barijera;

void* thr_f( void* arg )
{
//    int indeks       		        = ( (thr_struct*) arg )->indeks;

    int pocetak_po_red 		        = ( (thr_struct*) arg )->pocetak_po_red;
	int kraj_po_red                 = ( (thr_struct*) arg )->kraj_po_red;
//	int duljina_po_red              = kraj_po_red - pocetak_po_red +1;

    int k,l;

	for(k=pocetak_po_red; k<=kraj_po_red; ++k)
	    for(l=0; l<n; ++l)
            if(matrica[k*n+l] == 'o')
            {
                pthread_mutex_lock(&lock);
                //printf("(%d,%d)\n", k,l);
                brojac++;
                pthread_mutex_unlock(&lock);
            }

    pthread_barrier_wait(&barijera);

	return NULL;
}



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
    clock_t trajanje;
    trajanje = clock();

    if (argc != 5)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    char *ime_datoteke;
    int i, broj_dretvi, duljina;

    broj_dretvi = atoi(argv[1]);
    m = atoi(argv[2]);
    n = atoi(argv[3]);
    ime_datoteke = argv[4];
    duljina = m*n;    //duljina je velicina matrice plus jos jedan stupac za oznaku novog retka

    matrica = (char*)malloc(duljina*sizeof(char));
    ucitaj(duljina, ime_datoteke, matrica);

    trajanje -= clock();
    printf("Sekvencijalno ucitavanje traje  %f   sekundi\n", ((float)-trajanje)/CLOCKS_PER_SEC);
    trajanje = clock();

    pthread_t *thr_idx;
    pthread_barrier_init(&barijera, NULL, broj_dretvi);
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL)
    {
		fprintf(stderr, "%s: memory allocation error (greska kod alokacije dretvi)\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    nova = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct));

    printf("Pocetak algoritma:\n");

    int ponovno;
    for(ponovno=0; ponovno<10; ponovno++;)
    {
        for (i = 0; i < broj_dretvi; ++i)
        {
            nova[i].pocetak_po_red = ((i*m)/broj_dretvi);
            nova[i].kraj_po_red = (((i+1)*m)/broj_dretvi)-1;

            if (pthread_create(&thr_idx[i], NULL, thr_f,(void*)&nova[i]))
            {
                printf( "%s: error creating thread %d\n", argv[0], i);
                exit(EXIT_FAILURE);
            }
        }

        thr_struct *ptr;    //spajanje glavne dretve s novo napravljenima koje rade algoritam
        if (pthread_join(thr_idx[0], (void*)&ptr))
        {
            fprintf(stderr, "%s: error joining thread 0\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        printf("%d runda, pronadeno %d kampova\n", ponovno, brojac);
    }

    trajanje -= clock();
    printf("Trazenje kampova sa sekvencijalnim citanjem i %d dretvi treba   %f   sekundi\n", broj_dretvi, ((float)-trajanje)/CLOCKS_PER_SEC);

    return 0;
}


