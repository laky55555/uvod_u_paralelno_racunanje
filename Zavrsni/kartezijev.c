#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdlib.h>

///A matrica mora imat periods[1]=1
///B matrica mora imat periods[0]=1

int main(int argc, char **argv)
{

    int rank, size, i, j, dims[2], reorder=0, periods[2], coord[2], id, broj, source, dest;

    MPI_Status status;
    MPI_Comm novi_komunikator;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    periods[0] = 0;
    periods[1] = 1;
    dims[0] = dims[1] = (int) sqrt( (double) size );
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &novi_komunikator);
    broj = rank;

    for(i=0; i<size; ++i)
    {
        if(rank == i)
        {
            //printf("Rank %d\n", rank);
            MPI_Cart_coords(novi_komunikator, rank, 2, coord);
            printf("Rank %d coordinates are %d %d, i imam broj %d\n", rank, coord[0], coord[1], broj);fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Cart_shift(novi_komunikator, 1, -1, &source, &dest);
    MPI_Sendrecv_replace(&broj, 1, MPI_INT, dest, 2, source, 2,MPI_COMM_WORLD , &status);

    for(i=0; i<size; ++i)
    {
        if(rank == i)
        {
            //printf("Rank %d\n", rank);
            MPI_Cart_coords(novi_komunikator, rank, 2, coord);
            printf("Rank %d coordinates are %d %d i imam broj %d\n", rank, coord[0], coord[1], broj);fflush(stdout);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
   /* if (rank <55)
    {
        MPI_Cart_coords(novi_komunikator, rank, 2, coord);
        printf("Rank %d coordinates are %d %d\n", rank, coord[0], coord[1]);fflush(stdout);
    }
    if(rank <55)
    {
        coord[0]=3; coord[1]=1;
        MPI_Cart_rank(novi_komunikator, coord, &id);
        printf("The processor at position (%d, %d) has rank %d\n", coord[0], coord[1], id);fflush(stdout);
    }

*/


    MPI_Finalize();

    return 0;
}
