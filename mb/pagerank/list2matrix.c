#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv){

FILE *fnodes;
char nodes_file[1000];
FILE *flist;
char  list_file[1000];
FILE *fmatrix;
char  matrix_file[1000];
int i,j;
int **E;
char *path;
int N; // number of nodes

 /*** 
	The argument for the program is the directory name 
      	of the query for which we want to create the adjacency matrix
 ***/

 if (argc != 2){ 
	printf("list2matrix query_directory\n");
	exit(1);
 }

 path = strdup(argv[1]);


/*** open the nodes file to obtain the number of nodes ***/

 sprintf(nodes_file,"%s/graph/nodes",path);
 fnodes = fopen(nodes_file,"r");
 if (fnodes == NULL){
     printf("ERROR: Cant open file %s\n",nodes_file);
     exit(1);
 }
 fscanf(fnodes,"%d",&N);

 fclose(fnodes);


/**** Read List and Construct the adjacency matrix E ****/

 E = (int **)malloc(N*sizeof(int *));
 for (i = 0; i < N; i ++){
	  E[i] = (int *)malloc(N*sizeof(int));
	  for (j = 0; j < N; j ++){
			E[i][j] = 0;
	  }
 }

 sprintf(list_file,"%s/graph/adj_list",path);
 flist = fopen(list_file,"r");
 for (i = 0; i < N; i ++){
     fscanf(flist,"%*d: %d",&j);
     while (j != -1){
			E[i][j] = 1;
			fscanf(flist,"%d",&j);
	  }
 }

 fclose(flist);


 /*** print the adjacency matrix ***/

 sprintf(matrix_file,"%s/graph/adj_matrix",path);
 fmatrix = fopen(matrix_file,"w");
 for (i = 0; i < N; i ++){
        for (j = 0; j < N; j ++){
        	 if( j == N-1)
        	 {
        	 	// last entry has no comma seperation
        	 	fprintf(fmatrix,"%d", E[i][j]);
        	 }else
        	 {
             	fprintf(fmatrix,"%d,", E[i][j]);
             }
        }
	fprintf(fmatrix,"\n");
 }
 fclose(fmatrix);
}

