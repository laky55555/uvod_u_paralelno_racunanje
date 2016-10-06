#include <stdio.h>
#include <mpi.h>
#include <complex.h>
#include <math.h>
#include <stdlib.h>

///mnozenje kompleksnih matrica
extern void zgemm_(char *TRANSA, char *TRANSB, int *M, int *N, int *K, double _Complex *ALPHA, double _Complex *A, int *LDA, double _Complex *B, int *LDB, double _Complex *BETA, double _Complex *C, int *LDC);

///Ucitava matricu paralelno po blokovima (matrica je u binarnoj datoteci spremljena po retcima)
void ucitaj(double _Complex* matrica, char *datoteka, int rank, int size, int red_matrice, int red_blok_matrice, int broj_elemenata_bloka, int broj_blokova_po_retku)      ///ucitava blok matrice za svaki proces
{
    int i, pocetni_element;
    MPI_File file_pointer;
    MPI_Status status;

    MPI_File_open(MPI_COMM_WORLD, datoteka, MPI_MODE_RDONLY, MPI_INFO_NULL, &file_pointer);

    ///pocetni element ucitavanja je pozicija prvog elementa blok matrice
    pocetni_element = (rank/broj_blokova_po_retku) * broj_elemenata_bloka*broj_blokova_po_retku + rank%broj_blokova_po_retku*red_blok_matrice;

    ///ucitavanje krece od pocetnog elementa, onda se ucitava prvi redak blok matrice, zatim se skace u novi red velike (pocetne) matrice i ucitava slijedeci
    for (i=0; i<red_blok_matrice; ++i)
        MPI_File_read_at(file_pointer, (red_matrice*i+pocetni_element) * sizeof(double _Complex), matrica+i*red_blok_matrice, red_blok_matrice, MPI_C_DOUBLE_COMPLEX, &status);

    MPI_File_close(&file_pointer);
}

///Ispisuje dobivene blok matrice na odgovarajuce pozicije u binarnu datoteku tako da konstruira zavrsnu matricu
///Analogno kao i kod citanja, razlika je jedino MPI_File_read_at() u MPI_File_write_at()
void ispis(double _Complex* matrica, char *datoteke, int rank, int size, int red_matrice, int red_blok_matrice, int broj_elemenata_bloka, int broj_blokova_po_retku)      ///ucitava blok matrice za svaki proces
{
    int i, pocetni_element;
    MPI_File file_pointer;
    MPI_Status status;

    MPI_File_open(MPI_COMM_WORLD, datoteke, MPI_MODE_CREATE | MPI_MODE_WRONLY , MPI_INFO_NULL, &file_pointer);

    pocetni_element = (rank/broj_blokova_po_retku) * broj_elemenata_bloka*broj_blokova_po_retku + rank%broj_blokova_po_retku*red_blok_matrice;

    for (i=0; i<red_blok_matrice; ++i)
        MPI_File_write_at(file_pointer, (red_matrice*i+pocetni_element) * sizeof(double _Complex), matrica+i*red_blok_matrice, red_blok_matrice, MPI_C_DOUBLE_COMPLEX, &status);

    MPI_File_close(&file_pointer);
}


int main(int argc, char **argv)
{

    double _Complex *A, *B, *C, temp, alfa=1; ///matrice i pomocni element za transponirati i alfa za zgemm_()
    int i, j, red_matrice, red_blok_matrice, broj_elemenata_blok_matrice, broj_blokova_po_retku; ///potrebno za manipulacije s matricama
    int rank, size, source, dest, dims[2], reorder=0, periods_A[2], periods_B[2], coord[2], id, pomak;  ///za potrebe MPI-a
    char trans = 'T';   ///potrebno za lapack T jer je matrica spermljena po retcima

    MPI_Status status;
    MPI_Comm komunikator_A, komunikator_B;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    red_matrice = atoi(argv[4]);

    if (rank == 0)
    {
        if (argc != 5)
        {
            fprintf(stderr, "usage: %s A.dat, B.dat, C.dat, red matrica \n", argv[0]);
            fprintf(stderr, "A.dat - binarna datoteka, sadrzi double complex kvadratnu matricu, spremljena po retcima\n");
            fprintf(stderr, "B.dat - binarna datoteka, sadrzi double complex kvadratnu matricu, spremljena po retcima\n");
            fprintf(stderr, "C.dat - binarna datoteka, sadrzi double complex kvadratnu matricu, spremljena po retcima\n");
            fprintf(stderr, "int koji oznacava broj stupaca/redaka matrica\n");

            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        double a = size;
        int provjera;
        a = sqrt(a);
        provjera = a;

        if(provjera*provjera != size)
        {
            fprintf(stderr, "Broj procesa mora biti puni kvadrat\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

        if (red_matrice%provjera != 0)
        {
            fprintf(stderr, "Red matrice mora biti djeljiv s korjenom broja procesa\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }

    }

    broj_elemenata_blok_matrice = red_matrice*red_matrice/size;
    broj_blokova_po_retku = (int) sqrt( (double)size);
    red_blok_matrice = (int) sqrt( (double)broj_elemenata_blok_matrice);

    A = (double _Complex *) calloc( broj_elemenata_blok_matrice, sizeof(double _Complex));
    B = (double _Complex *) calloc( broj_elemenata_blok_matrice, sizeof(double _Complex));
    C = (double _Complex *) calloc( broj_elemenata_blok_matrice, sizeof(double _Complex));

    ucitaj(A, argv[1], rank, size, red_matrice, red_blok_matrice, broj_elemenata_blok_matrice, broj_blokova_po_retku);
    ucitaj(B, argv[2], rank, size, red_matrice, red_blok_matrice, broj_elemenata_blok_matrice, broj_blokova_po_retku);

    ///Matrica a je periodicna po retcima, a B po stupcima
    periods_A[0] = periods_B[1] = 0;
    periods_A[1] = periods_B[0] = 1;
    dims[0] = dims[1] = (int) sqrt( (double) size );

    ///stvara kartezijevu topologiju radi lakseg siftanja elemenata iz jednog u drugi proces
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods_A, reorder, &komunikator_A);
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods_B, reorder, &komunikator_B);

    ///radi pocetni pomak blokova (0 redak/stupac se pomicu 0, 1 redak/stupac za 1 mjesto, 2 za 2 mjesta...)
    MPI_Cart_shift(komunikator_A, 1, -rank/broj_blokova_po_retku, &source, &dest);
    MPI_Sendrecv_replace(A, broj_elemenata_blok_matrice, MPI_C_DOUBLE_COMPLEX, dest, 2, source, 2, MPI_COMM_WORLD, &status);

    MPI_Cart_shift(komunikator_B, 0, -rank%broj_blokova_po_retku, &source, &dest);
    MPI_Sendrecv_replace(B, broj_elemenata_blok_matrice, MPI_C_DOUBLE_COMPLEX, dest, 2, source, 2, MPI_COMM_WORLD, &status);

    ///mnozenje blok matrica
    zgemm_(&trans, &trans, &red_blok_matrice, &red_blok_matrice, &red_blok_matrice, &alfa, A, &red_blok_matrice, B, &red_blok_matrice, &alfa, C, &red_blok_matrice);

    ///radi pomake i mnozenja jos n-1 puta gdje je n broj blok matrica u stupcu/retku
    for(j=0; j<broj_blokova_po_retku-1; ++j)
    {
        MPI_Cart_shift(komunikator_A, 1, -1, &source, &dest);
        MPI_Sendrecv_replace(A, broj_elemenata_blok_matrice, MPI_C_DOUBLE_COMPLEX, dest, 2, source, 2, MPI_COMM_WORLD, &status);

        MPI_Cart_shift(komunikator_B, 0, -1, &source, &dest);
        MPI_Sendrecv_replace(B, broj_elemenata_blok_matrice, MPI_C_DOUBLE_COMPLEX, dest, 2, source, 2, MPI_COMM_WORLD, &status);

        zgemm_(&trans, &trans, &red_blok_matrice, &red_blok_matrice, &red_blok_matrice, &alfa, A, &red_blok_matrice, B, &red_blok_matrice, &alfa, C, &red_blok_matrice);
    }

    ///transponiranje matrice
    for(i=0; i<red_blok_matrice; ++i)
    {
        for(j=i; j<red_blok_matrice; ++j)
        {
            temp = C[j*red_blok_matrice+i];
            C[j*red_blok_matrice+i] = C[i*red_blok_matrice+j];
            C[i*red_blok_matrice+j] = temp;
        }
    }

    ispis(C, argv[3], rank, size, red_matrice, red_blok_matrice, broj_elemenata_blok_matrice, broj_blokova_po_retku);

    MPI_Finalize();

    return 0;
}
