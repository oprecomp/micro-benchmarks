/*
 * pca_.c
 *
 *  Created on: 20 gen 2016
 *      Author: Fabio
 */

// #include "hwPerf.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "pca_.h"
#include "dat.h"
#include <omp.h>
#include "init.h"


#define SIGN(a, b) ((b) >= 0.0 ? fabsf(a) : -fabsf(a))
#define MAX(x,y) ((x)>(y)?(x):(y))

#define DUMP_COUNTERS 1




 /*set the number of cores to execute the application*/

 int PCA(int lunghezza_finestra, int numero_canali, float output[][256]){ 
         
         float p[channels+1];
         float *rv1;
         float a[channels][channels]; //covariance matrix
         float w[channels]; //eigenvalues
         float v[channels][channels]; //eigenvectors
         float explained[channels];
         float anorm;
         int i, k; 
         rv1=p;
   
                 
         mean_covariance(datiInput2, a);  //compute mean value for al channels and substract mean value from datas 
         
     
        
        
         anorm=householder(a, rv1, w);  //Householder reduction to bidiagonal form
         
         accumulate(v, a, rv1);  //accumulate the right-hand transformation
       
         diagonalize(w, rv1, v, anorm);
         
         /*compute the k components*/
         
         float totalvar=0.0f;
         float vartotspiegata=0.0f;
         
         
         for(i=0; i<channels; i++)
                 
                 totalvar=totalvar+w[i]; 
         
         
         
         
         for(i=0; i<channels; i++)
                 
                 explained[i]=(100.0f*w[channels-i-1])/totalvar;
         
         
         
         
         i=channels;
         
         while(vartotspiegata<90){
                 
                 i=i-1;
                 vartotspiegata=vartotspiegata+explained[i];
                 
         }
         
         if(vartotspiegata>90){
                 
                 i=i+1;
                 vartotspiegata=vartotspiegata-explained[i];
                 
         }
         
         printf("\nexplained variance:\n%d\ncomponents: %d\n", (int)vartotspiegata,(channels-i));
         k=channels-i;
         printf("Number of components we consider: %d\n", k);
         
         //CALCOLO K PC
         
         //k_comp=k;
         int k_comp=9;
      
         PC(datiInput2, output, v);
         
         return k_comp;
         
 }

/*compute mean value for al channels and substract mean value from datas*/
void mean_covariance(float datiInput[][256], float a[][23]){
        
        int i, j, k;
        #pragma omp parallel default(none) private( i, k, j) shared(a, datiInput2) num_threads(CORE)
        {
                
                float temp=0.0f;
                #pragma omp for 
                
                for (j = 0; j < channels; j++){ 
                        
                        
                        float mean = 0.0f;
                        
                        for (i = 0; i < window; i++){
                                mean = mean + datiInput2[j][i];
                        }
                        mean /= window;
                        for (i = 0; i < window; i++){
                                datiInput2[j][i] -= mean;
                        }
                }
               
                
                /*compute covariance matrix*/   
                #pragma omp for collapse(2)
                for(i=0; i < channels; i++){
                        for(j=0; j < channels; j++){
                                
                                
                                for(k=0; k < window; k++){
                                        temp+=datiInput2[i][k]*datiInput2[j][k];
                                }
                                a[i][j]=temp; 
                                temp=0.0f; 
                        }
                }
        }//omp

}

/* Householder reduction to bidiagonal form */
float householder(float a[][23], float rv1[23], float w[23]){
        
        int i, its, j, jj, k, l, nm;
        float c, f, h, s, x, y, z, flag;
        float anorm = 0.0f, g = 0.0f;
        int somma=0;
        #pragma omp parallel default(none) private(s, g, k) shared(f, rv1, i, l, a, h, w, anorm) num_threads(CORE)       
        {
                
                for (i = 0; i < channels; i++)
                {
                         
                        // left-hand reduction 
                        #pragma omp master
                        {
                                l = i + 1;
                                rv1[i] = g;
                                g = 0.0f;
                                s = 0.0f;
                        }//master
                       
                        #pragma omp barrier                                                                                                                                                                  
                        
                        if (i < channels)
                        {
                                #pragma omp master
                                {
                                        
                                        for (k = i; k < channels; k++)
                                        {
                                                s += (a[k][i] * a[k][i]);
                                        }
                                            
                                        f = a[i][i];
                                        g = -SIGN(sqrtf(s), f); 
                                        h = f * g - s;
                                        a[i][i] = (f - g);
                                
                                }//master
                               

                                                      
                                #pragma omp barrier                                             
                                if (i != channels - 1)
                                {                     
                                        #pragma omp for private(f)                            
                                        for (j = l; j < channels; j++) 
                                        {       
                                                s = 0.0f;
                                                for ( k = i; k < channels; k++)
                                                         s += (a[k][i] * a[k][j]);
                                                                                
                                                f = s / h;
                                                                                
                                                for (k = i; k < channels; k++)
                                                        a[k][j]+= (f * a[k][i]);
                                        }
                                }
                                   
                        }
                        
                       
                        #pragma omp master
                        {
                              
                                w[i] = g; 
                                g = 0.0f;
                                s = 0.0f;
                             
                        }//master
                       
                        
                        if (i < channels && i != channels - 1)
                        {
                    
                                        #pragma omp master
                                        {
                                                                                
                                                        for (k = l; k < channels; k++)
                                                        {
                                                                s += (a[i][k] * a[i][k]);
                                                        }
                                                       
                                                        f = a[i][l];
                                                        g = -SIGN(sqrtf(s), f);
                                                        h = f * g - s;
                                                        h=1.0f / h; 
                                                        a[i][l] = (f - g);
                                                                         
                                                        for (k = l; k < channels; k++)
                                                                rv1[k] = a[i][k] * h;
                                        }//master 
                                       
                                        #pragma omp barrier
                                        if (i !=channels - 1)
                                        {
                                                              
                                                #pragma omp for                        
                                                for (j = l; j < channels; j++)
                                                {
                                                        s = 0.0f;
                                                                        
                                                        for ( k = l; k < channels; k++)
                                                                s += (a[j][k] * a[i][k]);
                                                                        
                                                                        
                                                         for (k = l; k < channels; k++)
                                                                 a[j][k]+= (s * rv1[k]);
                                                                        
                                                 }
                                         }

                        }

                        #pragma omp master
                        {
                                anorm = MAX(anorm, (fabsf(w[i]) + fabsf(rv1[i])));  
                        }     

                }
                
        }//omp

        return anorm;
}


/* accumulate the right-hand transformation */
void accumulate(float v[][23], float a[][23], float rv1[23]){
       
       int i, j, k, l;
       float s;
       float g = 0.0f;
       int somma=0.0f;
       #pragma omp parallel default(none) shared( a, v, g, l, rv1, i, j) private(k, s) num_threads(CORE) 
        {
                
                for (i = channels - 1; i >= 0; i--)
                {
                        if (i <channels - 1)
                        {
                              
                                 if (g)
                                 {  

                                        #pragma omp for
                                        for (j = l; j < channels; j++)
                                                v[j][i] = ((a[i][j] / a[i][l])/ g); 
                                        
                                        
                                        /* double division to avoid underflow */

                                        #pragma omp for 
                                        for (j = l; j < channels; j++)
                                        {
                                                s = 0.0f;
                                                
                                                for ( k = l; k < channels; k++)
                                                        s += (a[i][k] * v[k][j]);
                                                
                                                
                                                for (k = l; k < channels; k++){
                                                        v[k][j] += (s * v[k][i]);
                                                }
                                        }
                                           
                                }

                                        
                                        #pragma omp master
                                        {
                                        for (j = l; j < channels; j++)
                                                v[i][j] = v[j][i] = 0.0f;
                                        }
                                        #pragma omp barrier
                         }
                                         
                        #pragma omp master
                        {
                                v[i][i] = 1.0f;
                                g = rv1[i];
                                l = i;
                        }//master
                        #pragma omp barrier
                        
                 }
                 
        }//omp
     
  
}


/* diagonalize the bidiagonal form */


float PYTHAG(float a, float b)
{
        float at = fabsf(a), bt = fabsf(b), ct, result;
        
        if (at > bt)       { ct = bt / at; result = at * sqrtf(1.0f + ct * ct); }//printf("%d\t",(int)result);}
        else if (bt > 0.0f) { ct = at / bt; result = bt * sqrtf(1.0f + ct * ct); }//printf("%d\t",(int)result);}
        else result = 0.0f;
        
        return(result);
        
}

void diagonalize(float w[23], float rv1[23], float v[][23], float anorm){
        
        int i, its, j, jj, k, l, nm;
        float c, f, h, s, x, y, z;
        float g = 0.0f, scale = 0.0f;
        
        #pragma omp parallel default(none) private( g, h, f, j, y, z, x) shared(nm, k, its, l, rv1,  w, c, s, i, v, anorm) num_threads(CORE)//private(i,jj,j,y,z,x,h,g,s,c,f) shared(nm,rv1,w,v,m,n,a,flag) num_threads(CORE)// 
        {
                
                for (k =channels - 1; k >= 0; k--)
                {  

                        /* loop over singular values */
                        for (its = 0; its < 30; its++)
                        {  
                                /* loop over allowed iterations */
                               
                                #pragma omp master
                                {               
                                        
                                        for (l = k; l >= 0; l--)
                                        {  
                                                nm = l - 1;   
                                                if (fabsf(rv1[l]) + anorm == anorm || fabsf(w[nm]) + anorm == anorm)
                                                        break;
                                        }
                                }//master
                                
                                #pragma omp barrier
      
                                if (l == k)
                                {                  
                                        /* convergence */
                                        break;
                                        
                                }
                                #pragma omp barrier
                                #pragma omp master
                                {
                                        /* shift from bottom 2 x 2 minor */
                                        z = w[k];
                                        y = w[nm];
                                        x = w[l];
                                        nm = k - 1;     
                                        g = rv1[nm];
                                        h = rv1[k];
                                        
                                        f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0f * h * y);
                                        g = PYTHAG(f, 1.0f); 
                                        f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g, f))) - h)) / x; 
                                        
                                        /* next QR transformation */
                                        c = s = 1.0f;
                                }//master
                                #pragma omp barrier     
                                
                                for (j = l; j <= nm; j++)
                                {
                                        
                                        #pragma omp master
                                        {                                       
                                                
                                                i = j + 1;
                                                g = rv1[i];
                                                y = w[i];
                                                h = s * g;
                                                g = c * g;
                                                z = PYTHAG(f, h);
                                                rv1[j] = z;
                                                c = f / z;
                                                s = h / z;
                                                f = x * c + g * s;
                                                g = g * c - x * s;
                                                h = y * s;
                                                y = y * c;
                                                
                                                
                                        }//master
                                        #pragma omp barrier
                                        
                                        #pragma omp for //nowait
                                        for (jj = 0; jj < channels; jj++)
                                        {
                                                x = v[jj][j];
                                                z = v[jj][i];
                                                v[jj][j] = (x * c + z * s);
                                                v[jj][i] = (z * c - x * s);
                                        }
                                        
                                        #pragma omp master
                                        {
                                                z = PYTHAG(f, h);
                                                
                                                w[j] = z;
                                                
                                                if (z)
                                                {
                                                        z = 1.0f / z;
                                                        c = f * z;
                                                        s = h * z;
                                                        
                                                }
                                                
                                                f = (c * g) + (s * y);
                                                x = (c * y) - (s * g);
                                        }//master
                                        #pragma omp barrier
                                        
                                }
                                
                                
                                #pragma omp master
                                {
                                        rv1[l] = 0.0f;
                                        rv1[k] = f;
                                        w[k] = x; 
                                }               
                                
                        }
                        #pragma omp barrier
                }      
                
        }//omp
        
}

//CALCOLO K PC
void PC(float datiInput[][256], float datioutput[][256], float v[][23]){
        
        int i, k, k2;
        int k_comp=9;
        float temp;

        #pragma omp parallel default(none) private(k,k2, temp) shared(datiInput, v, datioutput, k_comp) num_threads(CORE) 
        {
                temp=0.0f;
                #pragma omp barrier
                #pragma omp for 
                for (i = 0; i < window; i++) {
                        
                        for(k=0; k < k_comp; k++) {
                                
                                for (k2 = 0; k2 < channels; k2++) 
                                {                                                                                                                                                               
                                        
                                        temp+= datiInput[k2][i] * v[k2][k];
                                        
                                }
                                datioutput[k][i]=temp; 
                                temp=0.0f;                      
                        } 
                }
                
        }//omp
        
}
