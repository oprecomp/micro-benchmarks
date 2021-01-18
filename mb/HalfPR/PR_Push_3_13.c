/* 
   PageRank - Push Style  
   Customized Half-Precision Floating-Point Format (3 exponent bits + 13 mantissa bits) used for data storage 
   IEEE Single-Preicsion Floating-Point Format used for computations
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#define MANTISSA_F 0x007fffff
#define EXPONENT_F 0x7f800000
#define MANTISSA_D 0x000fffffffffffff
#define EXPONENT_D 0x7ff0000000000000
#define MANTISSA_H 0x1fff
#define EXPONENT_H 0xe000

// ------------------ union to convert float number to unsigned integer and applying bit manipulation on it
union uint32_float
{
    float value;
    uint32_t uint_value;
};
//--------------------------------
uint16_t exp_encoder_t_16(float a, int a_index, int n, uint8_t *exception_value, int *exception_index, int *exception_count)
{
   int index, exception_end=*exception_count;
   bool flag=false;
   float E=1.0/n;
   union uint32_float one_over_n={E};    
   union uint32_float num={a};    
   uint32_t three_exp=3;
   uint16_t exception_seven=7;     
   three_exp = three_exp << 23;
   exception_seven = exception_seven << 13;
   uint32_t temp=0; 
   uint32_t exp_1_n  = one_over_n.uint_value & EXPONENT_F;
   uint32_t exp_num_in  = num.uint_value & EXPONENT_F;
   uint32_t mantissa_in = num.uint_value & MANTISSA_F;
   uint16_t exp_num_out;
   // ---- rounding mantissa : round_to_nearest_ties_up (round cof= 2^9)
   mantissa_in = mantissa_in + 512;   
   uint16_t mantissa_out = mantissa_in >> 10;      
   if (num.uint_value == 0 ) return(0);      
   if ( exp_num_in <= (exp_1_n + three_exp)) 
   {
     temp = exp_num_in - exp_1_n + three_exp;
     exp_num_out = temp >> 10;
   }
   else
   {
    exp_num_out = exception_seven;
    temp = exp_num_in - exp_1_n;
    temp = temp >> 23;
    index=exception_end;
    for(int j=0; j < exception_end; j++) 
        if (exception_index[j]== a_index)  {index=j; flag=true;}            
    exception_value[index]=temp;
    exception_index[index]=a_index;
    
    if (flag == false) *exception_count = exception_end +1;
   } 
   uint16_t output_t_16 = exp_num_out | mantissa_out;    
   return(output_t_16); 
} 
//--------------------------------- 
float exp_decoder_t_16(uint16_t a, int a_index, int n, uint8_t *exception_value, int *exception_index, int *exception_count)
{
   int exception_end=*exception_count;
   float E=1.0/n;
   union uint32_float one_over_n={E};    
   union uint32_float num={0};
   uint32_t three_exp=3;
   uint16_t exception_seven=7;    
   three_exp = three_exp << 23;
   exception_seven = exception_seven << 13;  
   uint32_t exp_1_n  = one_over_n.uint_value & EXPONENT_F;
   uint16_t exp_num_in  = a & EXPONENT_H;
   uint16_t mantissa_in = a & MANTISSA_H;
   uint32_t exp_num_out;
   uint32_t mantissa_out = mantissa_in << 10;
   uint32_t temp=0;  
   if (a == 0 ) return (0.0);                
   if ( exp_num_in != exception_seven) 
   {
      exp_num_out = exp_num_in << 10;
      exp_num_out = exp_num_out + exp_1_n - three_exp;
   }
   else
   {
       for(int j=0; j < exception_end; j++) 
           if (exception_index[j]== a_index)  temp=exception_value[j];             
       exp_num_out = temp << 23;
       exp_num_out = exp_num_out + exp_1_n;        
   }   
   num.uint_value = exp_num_out | mantissa_out; 
   return(num.value); 
}
//--------------------------------- 
float contribution_t_16(int n, float d, int out_link, uint16_t a_t_16, int a_t_16_index, uint8_t *exception_value, int *exception_index, int *exception_count)
{
   float p = exp_decoder_t_16(a_t_16, a_t_16_index, n, exception_value, exception_index, exception_count);
   float contribution = d*(p/out_link);
   return (contribution);   
}
//--------------------------------- 
void Load_t_16(uint16_t *p_t_16, int n) 
{
   float E = 1.0 / n;
   union uint32_float num={E};
   uint16_t three=3, one_over_n_t_16;   
   uint16_t exponent = three << 13;
   num.uint_value = num.uint_value & MANTISSA_F ;
   // ---- rounding mantissa_in to round_to_nearest_ties_up (round cof= 2^9)
   num.uint_value = num.uint_value + 512;   
   uint16_t mantissa = num.uint_value >> 10;   
   one_over_n_t_16 = exponent | mantissa;   
   for(int i=0; i < n; i++ )     p_t_16[i]=one_over_n_t_16;
}
//--------------------------------- 
void Store_t_16(uint16_t *p_t_16, float *p_new_f, int n, uint8_t *exception_value, int *exception_index, int *exception_count) 
{
   for (int i=0; i< n; i++)
   {
     p_t_16[i]=exp_encoder_t_16(p_new_f[i], i, n, exception_value, exception_index, exception_count);
     p_new_f[i]=0.0;   
   }
}
//---------------------------------
float normdiff_t_16(uint16_t *p_t_16, float *p_new_f, int n, uint8_t *exception_value, int *exception_index, int *exception_count)  
{
  float d=0.0, p;
  float err = 0.0;
  float tmp, y;
  for(int i=0; i < n; ++i ) 
  {
     //d += fabs( b[i] - a[i] );
     p = exp_decoder_t_16(p_t_16[i], i, n, exception_value, exception_index, exception_count);
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
  // --------------------------------  datasets file should be in compressed-sparse-row (CSR) format
    char filename[]="pkc_csr.txt";
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
  uint8_t  *exception_value=calloc(n/100, sizeof(uint8_t));
  int      *exception_index=calloc(n/100, sizeof(int));
  int       exception_count=0;
  float d_f=0.85, scale_f, delta_f, error_threshold;
  int looping = 1;  
  int rowel = 0;
  int curcol = 0;      
  float w_f;
  float  contribution_f=0; 
  int start, end;
  
  // ------ Reading nodes and edges according to the csr format  
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
  row_ptr[n] = e;    
  for(i=0; i<n; i++)
  {
     rowel = row_ptr[i+1] - row_ptr[i];
     if (rowel==0)
       out_link[i] = n;
     else
       out_link[i] = rowel;
  }

 // ----- default values 
  d_f = 0.85;  
  error_threshold=1e-4f; 
  iter=0;
  max_iter=200;
  delta=2; 
  for(i=0; i<n; i++)    p_new_f[i]=0.0;
  // ----- writing the default value 1/n to p_t in the proposed 16-bit format  
  Load_t_16(p_t_16, n);
  
//*************************** PageRank **************************
  while (delta_f>error_threshold && iter<max_iter)
  {
    rowel = 0;
    curcol = 0; 
    for(i=0; i<n; i++)
    {
      start = row_ptr[i];
      end = row_ptr[i+1];         
      contribution_f = contribution_t_16(n, d_f, out_link[i], p_t_16[i], i, exception_value, exception_index, &exception_count);
      for (j=start; j<end; j++) 
      {         
       p_new_f[col_ind[curcol]] = p_new_f[col_ind[curcol]] + contribution_f;   
       curcol++;
      }
    }
    // Scale page rank values w= (1-s)/n
    w_f = 1.0 - sum_f(p_new_f, n);
    scale_f = w_f * (1.0 / n);     
    for(i=0; i < n; i++ )    p_new_f[i] += scale_f;
    delta_f = normdiff_t_16(p_t_16, p_new_f, n , exception_value, exception_index, &exception_count);    
    fprintf(file, "\n Iteration %d: delta_f = %.15f", iter, delta_f);
    printf("\n Iteration %d: delta_f = %.15f", iter, delta_f);     
    Store_t_16(p_t_16, p_new_f, n, exception_value, exception_index, &exception_count);   
    iter = iter + 1;
}
// -------------------------------------------------- 
    fprintf(file, "\n\n--------------------------------------------------------------------------"); 
    fprintf(file, "\nFinished with %d iterations (threshold=%e)", iter, error_threshold);
    fprintf(file, "\nResidual error=%.20f", delta_f);  
    printf("\nFinished with %d iterations!\n\n", iter);

return 0;
}
