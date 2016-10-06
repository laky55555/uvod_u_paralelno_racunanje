#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

void read_matrix_binary(char *from_path,int rnum,int cnum,double *to){
	FILE *f = fopen(from_path,"rb");
	 if(f == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", from_path);
	int i,j;
	fread(to, sizeof(double), rnum*cnum, f);


	fclose(f);
}


void write_solution(char *to_path,int rnum,int cnum,int *from){
	FILE *f = fopen(to_path,"w");
	 if(f == NULL)
    	printf("Greska pri otvaranju datoteke %s\n", to_path);
	int i,j;
	//printf("write");
	for(i =0;i < rnum;++i)
		for(j = 0;j < cnum;++j){
			fprintf(f,"redak %d :  %d\n",i,from[i*cnum+j]);
			//printf("%lf",(double)i);
		}


	fclose(f);


}



//svakoj drtvi posalji svoj dio matrice i neka ga ona ispise

int main(int argc, char **argv)
{
	int rank;
	int nproc;

	int N = 8;
	int *A;
	int r=2;

	MPI_Datatype matrix;
	int block_num =  N/r ;


	MPI_Init(NULL, NULL);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);

	if (rank == 0) {
		if ((A = (int *) calloc(N * N, sizeof(int))) == NULL) {
			fprintf(stderr, "%s: %d: memory allocation failure\n", argv[0], rank);
			MPI_Abort(MPI_COMM_WORLD, 0);
		}

		for (int i = 0; i < N * N; ++i)
			A[i] = i;

		// kreiramo tip kojim želimo poslati glavni vodeći rxr blok s LDA N
		MPI_Type_vector(r, r, N, MPI_INTEGER, &matrix);
		MPI_Type_commit(&matrix);

		//svaki bilo svome
		for(int i = 0;i < block_num;++i)
        {
			for(int j = 0;j < block_num;++j)
            {
                MPI_Send(&A[(i)*2*N+j], 1, matrix,((block_num)*i+j)%(nproc -1)+1, 0, MPI_COMM_WORLD);
                //printf("to %d\n",((block_num)*i+j)%(nproc -1)+1);
			}

        }
	} else {
		if ((A = (int *) calloc(r * r, sizeof(int))) == NULL) {
			fprintf(stderr, "%s: %d: memory allocation failure\n", argv[0], rank);
			MPI_Abort(MPI_COMM_WORLD, 0);
		}

		// primamo rxr matricu s LDA r
		MPI_Type_vector(r, r, r, MPI_INTEGER, &matrix);
		MPI_Type_commit(&matrix);
		int broj = block_num*block_num;
		broj = broj/(nproc-1);

		//najveci broj bloka je (block_num)*(block_num)-1
		int tst = ((block_num)*(block_num))%(nproc -1);

		if((tst!=0) && (rank <= tst)){
			broj += 1;
		}
		printf("rank %d. dobiva %3d bloka\n",rank, broj);
		for(int i = 0;i < broj;++i){
		MPI_Recv(A, 1, matrix, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			char to[30];
			sprintf(to, "%d_%d.txt", rank,i);
			//for (int i = 0; i < r; ++i) {
				//for (int j = 0; j < r; ++j)
					//printf("%3d", A[i * r + j]);

				//printf("\n");
			//}
			write_solution(to,r,r,A);


		}

		}

	MPI_Type_free(&matrix);
	free(A);

	MPI_Finalize();
	return 0;
}
