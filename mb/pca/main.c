#include "omp.h"
#include <stdio.h>
#include <stdlib.h>
#include "pca_.h"
#include "wavelet_.h"
#include "libSVM_predict.h"
#include "libSVM_load_model.h"
#include "hwPerf.h"
#include "utils.h"
#include "init.h"
// #include "dat.h"

#define HWPERF 1

#define DUMP_COUNTERS 1

//TO SELECT THE NUMBER OF CORES FOR THE MULTI-CORE EXECUTION JUST CHANGE THE DEFINED VARIABLE "CORE" IN init.h
//TO EXECUTE THE MULTI-CORE APPLICATION COMPILE AND RUN WITH: make clean all run pulpFpu=1 pulpDiv=1 
//FOR THE 8 CORES ONLY: make clean all run pulpFpu=1 pulpDiv=1 nbPe=8
//TO EXECUTE THE APPLICATION IN SEQUENTIAL COMPILE AND RUN WITH: make clean all run pulpFpu=1 pulpDiv=1 -f Makefile.seq  

void PCA_mrrr(int samples, int variables, float *input, int components, float *output);

float datiOutput[channels][window];
PULP_L1_DATA extern float datiInput2[channels][window];

PULP_L1_DATA int max_nr_attr = 64;
PULP_L1_DATA int nr_attr = 37;


PULP_L1_DATA extern struct svm_node *x;
PULP_L1_DATA extern struct svm_model *model;
PULP_L1_DATA float *dec_values;

PULP_L1_DATA float target_label, predict_label;


PULP_L1_DATA int components;
	
PULP_L1_DATA int i;
PULP_L1_DATA int lev=0;
PULP_L1_DATA int j=0;

PULP_L1_DATA float energy_matrix[4][channels];
PULP_L1_DATA float *ptr_energy_vector;
PULP_L1_DATA float energy_vector[4];
PULP_L1_DATA float dwt[window];


int main()
{
        
        #ifdef SEQ
        if(get_core_id())
                return 0;
        #endif
        
        #if HWPERF
        hw_perf_t perf;
        hw_perf_init(&perf);
        
        while (hw_perf_step(&perf))
        {
                //hw_perf_start(&perf);
                #endif
                
              
                svm_node x1[37]; 
                //PCA: call the funcion for Principal Component Analysis. components is the number of Principal Component we consider. 
     
                hw_perf_start(&perf);
#if 0
		components=PCA(window, channels, datiOutput);
#else
                components = 9;
                PCA_mrrr(window, channels, (float *)datiInput2, components, (float *)datiOutput);
#endif
            	hw_perf_stop(&perf);
                hw_perf_commit(&perf);
            
                
                ptr_energy_vector=energy_vector;
                
                //DWT: call the function gsl_wavelet_transform for all the PC. The result is in the vector dwt. 
                //Energy: call the funcion calcolo_energia. We compute the energy of dwt. The result goes in energy_matrix.  

         
                #pragma omp parallel private(dwt, i, energy_vector) shared(energy_matrix) num_threads(CORE)//shared(energy_matrix)
                {
                        
                        #pragma omp for
                        for (j=0; j<components; j++){
                                int indx=0;
                                
                                for(indx=0; indx<window; indx++){
                                        
                                        dwt[indx]=datiOutput[j][indx]; //printf("%d:iter %d, dwt[%d]=%d\n", omp_get_thread_num(), j, indx, (int)dwt[indx]);     
                                        
                                }
                                
                                gsl_wavelet_transform (dwt, 1, window);
                                
                                
                                calcolo_energia(dwt, energy_vector);
                                
                                
                                
                                
                                for(i=0;i<4;i++){
                                        
                                        energy_matrix[i][j]=energy_vector[i]; //printf("%d\t\t", (int)energy_vector[i]);
                                        x1[((j)*4)+i].index=((j)*4)+i+1;
                                        x1[((j)*4)+i].value=energy_vector[i]/100000.0f; //printf("\nX:%d\n", (int)x1[((j-1)*4)+i].value);
                                        
                                }
                                
                                
                        }
                        #pragma omp barrier     
                }//omp
               
                
                x1[36].index=-1;
                x1[36].value=0.0f;
                
        
                
                printf("\nENERGY MATRIX\n\n");
                for(j=0;j<4;j++){
                        for(i=0;i<components;i++){
                                
                                printf("%d\t", (int)energy_matrix[j][i]);
                                
                        }printf("\n");
                }
                
                
                //SVM: call the funcion svm_load_model , svm_init_data and svm_predict_values to perform the classification.
                
                svm_load_model();
                
                dec_values = (float*) malloc(sizeof(float)*model->nr_class*(model->nr_class-1)/2);
              
            
              
                svm_predict_values( x1, dec_values);
       
                
                #if HWPERF      
                // hw_perf_stop(&perf);
                // hw_perf_commit(&perf);
        }
        
        
        printf("HW performance counters results\n");
        
        for (i=0; i<hw_perf_nb_events(&perf); i++)
        {
                printf("p: %s %d\n", hw_perf_get_name(&perf, i), hw_perf_get_value(&perf, i));
        }
        #endif  
        
        return 0;
}


