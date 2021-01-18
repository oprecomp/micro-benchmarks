/* 
   PageRank - Pull Style  
   Customized Half-Precision Floating-Point Format (6 exponent bits + 10 mantissa bits) used for data storage 
   IEEE Single-Preicsion Floating-Point Format used for computations
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <x86intrin.h>
#define EXPONENT_H 0x00010000

// ------------------ union to convert float number to unsigned integer and applying bit manipulation on it
union uint32_float
{
    float value;
    uint32_t uint_value;
};
//--------------------------------
uint16_t encode(float p)
{
   union uint32_float num = {p};
   num.uint_value = num.uint_value + 4096;   
   num.uint_value = (num.uint_value >> 13); 
   uint16_t output = num.uint_value;
   return(output); 
}
//--------------------------------- 
float decode(uint16_t p_t_16)
{
   union uint32_float num;
   num.uint_value = p_t_16;
   num.uint_value = (num.uint_value | EXPONENT_H);
   num.uint_value = (num.uint_value << 13); 
   return(num.value); 
}
//--------------------------------- 
void Load(uint16_t *p_t_16, float *p_new_f, int n) 
{
   float E = 1.0 / n;
   union uint32_float num={E};
   uint16_t one_over_n_t_16;
   // ---- rounding mantissa_in - round_to_nearest_ties_up (round cof= 2^12)
   num.uint_value = num.uint_value + 4096;   
   num.uint_value = (num.uint_value >> 13);   
   one_over_n_t_16 = num.uint_value; 
   for(int i=0; i < n; i++ )        
   {
     p_t_16[i]=one_over_n_t_16;
     p_new_f[i]=0;
   }          
}
//---------------------------------
float normdiff_t_16(uint16_t *p_t_16, float *p_new_f, int n)  
{
float d=0.0, p;
float err = 0.0;
float tmp, y;
for(int i=0; i < n; ++i ) 
{
    //d += fabs( b[i] - a[i] );
    p = decode(p_t_16[i]);
    tmp = d;
    y = fabs( p_new_f[i] - p ) + err;
    d = tmp + y;
    err = tmp - d;
    err += y;
}
return (d);
}
//---------------------------------
float sum_f( float a[], int n ) 
{
float d = 0.0;
float err = 0.0;
int i=0;
float tmp, y;
for(i=0; i < n; ++i )
{
	// d += a[i];
	tmp = d;
	y = a[i] + err;
	d = tmp + y;
	err = tmp - d;
	err  += y;
}
return d;
}
// *********************** Main program **********************
int main()
{ 
  int n, e;
  char ch;
  char str[100];
  FILE *fp, *file;
  // --------------------------------  datasets file should be in compressed-sparse-column (CSC) format
    char filename[]="pkc_csc.txt";
    char txtfile[] = "results_pkc.txt";
  //-----------------------------------------  Openning Dataset ....
  fp = fopen(filename,"r");
  if(!fp)
  {
    fprintf(stderr,"Error in openning the file!");
    exit(1);
  }  
  file = fopen(txtfile, "w");
  if(!file)
    {
        perror("Error in opennig a file to write result");
        exit(EXIT_FAILURE);
    }
   fscanf(fp,"%d%d",&n,&e);
   fprintf(file, "Dataset name: %s    \nNumber of Nodes = %d   \nNumber of Edges = %d\n",filename, n, e);
   printf("\nDataset name: %s    \n\nNumber of Nodes = %d   \nNumber of Edges = %d\n", filename, n, e);
     
// ********************** required variables ****************************  

  int *col_ind = calloc(e, sizeof(int));
  int *row_ptr = calloc(n+1, sizeof(int));
  int *out_link=calloc(n, sizeof(int));    
  int fromnode, tonode;
  int cur_row = 0;
  int i = 0, j = 0;
  int iter, max_iter, k, curel = 0;  
  uint16_t *p_t_16 =calloc(n, sizeof(uint16_t)); 
  float    *p_new_f=calloc(n, sizeof(float));  
  float d_f=0.85;
  float delta_f;
  float error_threshold;
  float scale_f;
  int looping = 1;  
  int rowel = 0;
  int curcol = 0;      
  float w_f;
  int start, end;
  
  // ------ Reading nodes and edges according to the csc format   
  for (i=0; i<n; i++)
  {
    fscanf(fp,"%d",&rowel);
    row_ptr[i]=rowel;
  }
  for (i=0; i<e; i++)
  {
    fscanf(fp,"%d",&curcol);
    col_ind[i]=curcol;
  }  
  //  Calculating total output links for each node (out_link[]) 
  row_ptr[n] = e; 
  
  for(i=0;i<n; i++) out_link[i]=0;
     
  for(i=0; i<e; i++)
  {     
       out_link[col_ind[i]]++;  
  }
	
  // ----- default values  
  d_f = 0.85;
  error_threshold=1e-3f; 
  iter=0;
  max_iter=200;
  delta_f=2;
  Load(p_t_16, p_new_f, n);
  
//*************************** PageRank **************************
  while (delta_f>error_threshold && iter<max_iter)
  {
    rowel = 0;
    curcol = 0; 
    for(i=0; i<n; i++)
    {
      start = row_ptr[i];
      end = row_ptr[i+1];             
      for (j=start; j<end; j++) 
      {   
       w_f = d_f / out_link[col_ind[j]];             
       p_new_f[i]   = p_new_f[i]   + w_f * decode(p_t_16[col_ind[j]]);         
      }
    }  
    // Scale page rank values w= (1-s)/n
    w_f = 1.0 - sum_f(p_new_f, n);
    scale_f = w_f * (1.0 / n);
    for(i=0; i < n; i++ )   p_new_f[i] += scale_f; 
    delta_f = normdiff_t_16(p_t_16, p_new_f, n);
    iter++;   
    w_f = 1.0 / sum_f(p_new_f, n);    
    for(i=0; i < n; i++ )   
    {
	p_t_16[i] = encode(p_new_f[i] * w_f);
	p_new_f[i] = 0;
    }         
    fprintf(file, "\n Iteration %d: delta = %.15f", iter, delta_f);
    printf("\n Iteration %d: delta = %.15f", iter, delta_f);   
}
//------------------------------------------------------------------------------------------
    fprintf(file, "\n\n--------------------------------------------------------------------------"); 
    fprintf(file, "\nFinished with %d iterations (threshold=%e)", iter, error_threshold);
    fprintf(file, "\nResidual error=%.20f", delta_f);  
    printf("\nFinished with %d iterations!\n\n", iter);
     
  return 0;
}
