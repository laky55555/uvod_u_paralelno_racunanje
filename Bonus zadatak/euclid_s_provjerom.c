#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>


extern void zaxpy_(int *N, double complex *ALFA, complex *x, int *INCX, double complex *y, int *INCY );
extern double complex zdotc_(int *N, double complex *X, int *INCX, double complex *Y, int *INCY);


void ucitaj (int duljina, char *ime_datoteke, double complex *x)  //ucitavanje vektora iz datoteke
{
    FILE *vektor_file;
    vektor_file = fopen(ime_datoteke, "rb");
    if(vektor_file == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", ime_datoteke);
    fread(x, sizeof(double complex), duljina, vektor_file);
    fclose(vektor_file);
}

void ispis_vektora(double complex *a, int x, int ime)  //ispis vektora (za provjeru)
{
    printf("%d:\n", ime);
    int k;
    for(k=0; k<x; ++k)
        printf("%lg+%lgi ", creal(a[k]), cimag(a[k]));
    printf("\n");
}


int main(int argc, char **argv)
{

    if (argc != 3)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    int	rank, broj_dretvi;
    int i, pocetni_indeks, duljina_komada, dimenzija_vektora;;
    double complex *x, najveci_element_komada;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &broj_dretvi);

    ///Svaki proces ima razlicit komad vektora, i tu se odredi tko ima koji dio
    dimenzija_vektora = atoi(argv[1]);
    pocetni_indeks = rank*dimenzija_vektora/broj_dretvi;
    duljina_komada = ((rank+1)*dimenzija_vektora)/broj_dretvi - pocetni_indeks;

    x = (double complex*)malloc(duljina_komada*sizeof(double complex));

    ///prvi proces ucitava vektor i dijeli ostalim procesima ono sta im treba
    if(rank==0)
    {
        double complex *cijeli_vektor;

        cijeli_vektor = (double complex*)malloc(dimenzija_vektora*sizeof(double complex));
        ucitaj(dimenzija_vektora, argv[2], cijeli_vektor);

        //dio za provjeru
        double complex suma=0;
        for (i=0; i<dimenzija_vektora; ++i)
            suma += (cabs(cijeli_vektor[i])*cabs(cijeli_vektor[i]));
        ispis_vektora(cijeli_vektor, dimenzija_vektora, rank);
        printf("Euklidska norma je: sqrt(%lg+%lgi) = %lg\n\n", creal(suma), cimag(suma), sqrt(suma));
        //kraj djela za provjeru

        for(i=0; i<duljina_komada; ++i)
            x[i] = cijeli_vektor[i];

        for (i=1; i<broj_dretvi; ++i)
        {
            int temp_pocetni_indeks = i*dimenzija_vektora/broj_dretvi;
            int temp_duljina_komada = ((i+1)*dimenzija_vektora)/broj_dretvi - temp_pocetni_indeks;
            MPI_Send(&cijeli_vektor[temp_pocetni_indeks],temp_duljina_komada, MPI_C_DOUBLE_COMPLEX, i, 0, MPI_COMM_WORLD);
        }

    }
    ///svi procesi osim prvog primaju svoje djelove vektora
    else
        MPI_Recv(x,duljina_komada,MPI_C_DOUBLE_COMPLEX,0,0,MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for(i=0; i<duljina_komada; ++i)
        if(i == 0 || cabs(najveci_element_komada) < cabs(x[i]) )
            najveci_element_komada = x[i];


    ///prode se po stablu tako da se uvijek desno djetet (proces) ostane radit, i u njemu ce ostat najveci element
    int potencija=1;
    while(potencija<broj_dretvi)
    {
        potencija *=2;

        ///saljemo poruku odgovarajucem cvoru
        if((broj_dretvi-1-rank)%potencija == 0)
        {
            if(rank - potencija/2 >= 0)  ///tu se provjerava da li postoji netko tko ce mu poslati element,  potencija/2 nam oznacava slijedeci cvor
            {
                double complex primljeni_broj;
                MPI_Recv(&primljeni_broj, 1, MPI_DOUBLE_COMPLEX, rank - potencija/2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if(cabs(primljeni_broj) > cabs(najveci_element_komada))
                    najveci_element_komada = primljeni_broj;
            }

        }
        else if (((broj_dretvi-1-rank)+potencija/2) % potencija == 0)
            MPI_Send(&najveci_element_komada, 1, MPI_DOUBLE_COMPLEX, rank + potencija/2 , 0, MPI_COMM_WORLD);

    }

    if(rank == broj_dretvi-1)
        printf("\n%d nasli smo najveci%lg+%lgi \n",rank, creal(najveci_element_komada), cimag(najveci_element_komada));


    ///Slanje najveceg elementa svim procesima
    while(potencija > 1)
    {

        if( (broj_dretvi-1-rank)%potencija == 0 && rank - potencija/2 >= 0)
            MPI_Send(&najveci_element_komada, 1, MPI_DOUBLE_COMPLEX, rank - potencija/2 , 0, MPI_COMM_WORLD);

        if( (broj_dretvi-1-rank)%potencija == potencija/2 )
            MPI_Recv(&najveci_element_komada, 1, MPI_C_DOUBLE_COMPLEX, rank + potencija/2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        potencija /=2;
    }


    int alfa = 1;
    double complex mnozitelj, y[duljina_komada], suma_komada=0;
    mnozitelj = 1/najveci_element_komada;


    /*for (i=0; i<duljina_komada; ++i)
    {
        x[i] *= mnozitelj;
        suma_komada += cabs(x[i])*cabs(x[i]);
    }*/

    zaxpy_( &duljina_komada, &mnozitelj, x, &alfa, y, &alfa);
    suma_komada =  zdotc_( &duljina_komada, y, &alfa, y, &alfa);

    ///treba jos vratiti sumu natrag po stablu
    potencija=1;
    while(potencija<broj_dretvi)
    {
        potencija *=2;

        ///saljemo poruku odgovarajucem cvoru
        if((broj_dretvi-1-rank)%potencija == 0)
        {
            if(rank - potencija/2 >= 0)  ///tu se provjerava da li postoji netko tko ce mu poslati element,  potencija/2 nam oznacava slijedeci cvor
            {
                double complex primljena_suma;
                MPI_Recv(&primljena_suma, 1, MPI_DOUBLE_COMPLEX, rank - potencija/2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                suma_komada += primljena_suma;
            }

        }
        else if (((broj_dretvi-1-rank)+potencija/2) % potencija == 0)
            MPI_Send(&suma_komada, 1, MPI_DOUBLE_COMPLEX, rank + potencija/2 , 0, MPI_COMM_WORLD);
    }


    if(rank==broj_dretvi-1)
        printf("Euklidska norma pomocu MPI je: |%lg+%lgi|*sqrt(%lg) = %lg\n\n\n", creal(najveci_element_komada), cimag(najveci_element_komada), creal(suma_komada), sqrt(creal(suma_komada))*cabs(najveci_element_komada));


	MPI_Finalize();




    return 0;
}
