#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>
#include <time.h>



int main(int argc, char **argv)
{
    clock_t trajanje;
    trajanje = clock();

    if (argc != 5)
    {
        fprintf(stderr, "Greska pri upisu komandne linije:\n%s \n", argv[0]);
		exit(EXIT_FAILURE);
    }

    char *ime_datoteke, znak;
    int m, n, i, broj_dretvi, thread_id, broj_citanja, nloops, vanjska=0;

    broj_dretvi = atoi(argv[1]);
    m = atoi(argv[2]);
    n = atoi(argv[3]);
    ime_datoteke = argv[4];

    FILE *matrice_file;
    matrice_file = fopen(ime_datoteke, "r");
    //for (i=0; i<duljina; ++i)
        //fscanf(matrice_file, "%lg", &x[i]);

    #pragma omp parallel private(thread_id, broj_citanja) num_threads(broj_dretvi)
    {
        broj_citanja=0;
        #pragma omp for private (znak, nloops)
        for (i=0; i<m*n+m; ++i)
        {

            fscanf(matrice_file, "%c", &znak);
            #pragma omp critical
            {
                nloops=vanjska;
                vanjska++;
                printf("%d\n",nloops);
            }
            broj_citanja++;
            if(znak == 'o')
            {
                #pragma omp critical
                {
                   // thread_id = omp_get_thread_num();
                    printf("Kamp je na (%d,%d)\n", nloops/(n+1), nloops%(n+1));
                }
            }
        }
        thread_id = omp_get_thread_num();

        printf("Thread %d performed %d iterations of the loop.\n", thread_id, broj_citanja);

      }

    fclose(matrice_file);

    return 0;
}
