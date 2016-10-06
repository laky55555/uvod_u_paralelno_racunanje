#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

char *matrica;
int m,n;

typedef struct _thr_struct{
	int pocetak, kraj, stupac;
} thr_struct;
thr_struct *nova;

pthread_mutex_t lock;
pthread_barrier_t barijera;

void* thr_f( void* arg )
{
    int pocetak		        = ( (thr_struct*) arg )->pocetak;
	int kraj                = ( (thr_struct*) arg )->kraj;
	int stupac              = ( (thr_struct*) arg )->stupac;

    int k,l;

	for(k=pocetak; k<=kraj; ++k)
        if(matrica[k] == 'o')
        {
            pthread_mutex_lock(&lock);
            printf("(%d,%d)\n", k/stupac,k%stupac);
            pthread_mutex_unlock(&lock);
        }

    pthread_barrier_wait(&barijera);

	return NULL;
}


/*void ucitaj (int redci, int stupci, char *ime_datoteke, char *x)  //ucitavanje matrice ako je matrica zapisana u jednom nizu u datoteki
{
    int i;
    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "r");
    for (i=0; i<duljina; ++i)
        fscanf(matrice_file, "%c", &x[i]);
    fclose(matrice_file);
}*/

void ucitaj (int redci, int stupci, char *ime_datoteke, char *x)  //ucitavanje matrice ako je matrica zapisana kao matrica, s znakovima za novi red
{
    int i, k=0;
    char znak;
    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "r");
    for (i=0; i<redci*stupci+stupci-1; ++i)
    {
        fscanf(matrice_file, "%c", &znak);
        if(znak == '.' || znak == 'o')
            x[k++]=znak;
    }
    fclose(matrice_file);
}

int main(int argc, char **argv)
{
    struct timeval stop, start;
    struct timeval stop1, start1;
    gettimeofday(&start, NULL);


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
    ucitaj(m, n, ime_datoteke, matrica);



    pthread_t *thr_idx;
    pthread_barrier_init(&barijera, NULL, broj_dretvi);
    if ((thr_idx = (pthread_t *) calloc(broj_dretvi, sizeof(pthread_t))) == NULL)
    {
		fprintf(stderr, "%s: memory allocation error (greska kod alokacije dretvi)\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    nova = (thr_struct*)malloc(broj_dretvi*sizeof(thr_struct));
    gettimeofday(&start1, NULL);
    for (i = 0; i < broj_dretvi; ++i)
    {
        nova[i].pocetak = ((i*m*n)/broj_dretvi);       //dijeljenje pozicija za pocetak trazenja po dretvama
        nova[i].kraj = (((i+1)*m*n)/broj_dretvi)-1;    //svaka dretva ima jedan dio niza za pretraziti
        nova[i].stupac = n;    //ovisno pretrazujemo li po redku ili stupcu

        if (pthread_create(&thr_idx[i], NULL, thr_f,(void*)&nova[i]))
        {
            printf( "%s: error creating thread %d\n", argv[0], i);
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_join(thr_idx[0], NULL))  //spajanje glavne dretve s novo napravljenima koje rade algoritam
    {
        fprintf(stderr, "%s: error joining thread 0\n", argv[0]);
        exit(EXIT_FAILURE);
    }

gettimeofday(&stop1, NULL);

    pthread_barrier_destroy(&barijera);
	free(thr_idx);
	free(matrica);
	free(nova);

     gettimeofday(&stop, NULL);
    long unsigned sec = stop.tv_sec-start.tv_sec, usec;
    if(stop.tv_usec > start.tv_usec)
        usec = stop.tv_usec - start.tv_usec;
    else
    {
        usec = 1000000-start.tv_usec+stop.tv_usec;
        sec--;
    }
    printf("\nukupan program sec: %lu, usec: %lu\n", sec, usec);
    long unsigned sec1 = stop1.tv_sec-start1.tv_sec, usec1;
    if(stop1.tv_usec > start1.tv_usec)
        usec1 = stop1.tv_usec - start1.tv_usec;
    else
    {
        usec1 = 1000000-start1.tv_usec+stop1.tv_usec;
        sec1--;
    }
    printf("dretve sec: %lu, usec: %lu\n", sec1, usec1);
    return 0;
}


