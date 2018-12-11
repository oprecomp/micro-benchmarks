#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include "hwPerf.h"
#include "modelSVM.h"
#include "libSVM_predict.h"
#include "libSVM_load_model.h"
#include <omp.h>

#include "utils.h"

#include "init.h"


PULP_L1_DATA struct svm_node *x;
PULP_L1_DATA struct svm_model *model;

PULP_L1_DATA int predict_probability=0;



void exit_input_error(int line_num)
{
        printf("Wrong input format at line %d\n", line_num);
        return;
}

void exit_with_help()
{
        printf(
                "Usage: svm-predict [options] test_file model_file output_file\n"
                "options:\n"
                "-b probability_estimates: whether to predict probability estimates, 0 or 1 (default 0); for one-class SVM only 0 is supported\n"
                "-q : quiet mode (no outputs)\n"
        );
        return;
}


void svm_free_model_content(svm_model* model_ptr)
{
        int i = 0;
        if(model_ptr->free_sv && model_ptr->l > 0 && model_ptr->SV != NULL)
                free((void *)(model_ptr->SV[0]));
        if(model_ptr->sv_coef)
        {
                for(i=0;i<model_ptr->nr_class-1;i++)
                        free(model_ptr->sv_coef[i]);
        }
        
        free(model_ptr->SV);
        model_ptr->SV = NULL;
        
        free(model_ptr->sv_coef);
        model_ptr->sv_coef = NULL;
        
        free(model_ptr->rho);
        model_ptr->rho = NULL;
        
        free(model_ptr->label);
        model_ptr->label= NULL;
        
        free(model_ptr->probA);
        model_ptr->probA = NULL;
        
        free(model_ptr->probB);
        model_ptr->probB= NULL;
        
        free(model_ptr->nSV);
        model_ptr->nSV = NULL;
}


void svm_free_and_destroy_model(svm_model** model_ptr_ptr)
{
        if(model_ptr_ptr != NULL && *model_ptr_ptr != NULL)
        {
                svm_free_model_content(*model_ptr_ptr);
                free(*model_ptr_ptr);
                *model_ptr_ptr = NULL;
        }
}




static inline float powi(float base, int times)
{
        float tmp = base, ret = 1.0;
        int t = 0;
        for(t=times; t>0; t/=2)
        {
                if(t%2==1) ret*=tmp;
                tmp = tmp * tmp;
        }
        return ret;
}


float dot(const svm_node *px, const svm_node *py)
{
        float sum = 0;
        while(px->index != -1 && py->index != -1)
        {
                if(px->index == py->index)
                {
                        sum += px->value * py->value;
                        ++px;
                        ++py;
                }
                else
                {
                        if(px->index > py->index)
                                ++py;
                        else
                                ++px;
                }                       
        }
        return sum;
}

float kernel_function(const svm_node *x, const svm_node *y, const svm_parameter param)
{
        /*
         *        switch(param.kernel_type)
         *        {
         *                case LINEAR:
         *                        return dot(x,y);
         *                case POLY:
         *                        return powi(param.gamma*dot(x,y)+param.coef0,param.degree);
         *                case RBF:
         *                {
         */
        float sum = 0;
        while(x->index != -1 && y->index !=-1)
        {
                if(x->index == y->index)
                {
                        float d = x->value - y->value;
                        sum += d*d;
                        ++x;
                        ++y; 
                }
                else
                {
                        if(x->index > y->index)
                        {       
                                sum += y->value * y->value;
                                ++y;
                        }
                        else
                        {
                                sum += x->value * x->value;
                                ++x;
                        }
                }
        }
        
        while(x->index != -1)
        {
                sum += x->value * x->value;
                ++x;
        }
        
        while(y->index != -1)
        {
                sum += y->value * y->value;
                ++y;
        }

        return expf(-param.gamma*sum);
        /*
}
case SIGMOID:
        return tanh(param.gamma*dot(x,y)+param.coef0);
case PRECOMPUTED:  //x: test (validation), y: SV
        return x[(int)(y->value)].value;
default:
        return 0;  // Unreachable 
}
*/
}


//float svm_predict_values(const svm_model *model, const svm_node *x, float* dec_values)
float svm_predict_values(const svm_node *x, float* dec_values)
{
        
        int j;
        int i;
        int cont=0;
        
        int nr_class = model->nr_class;
        int l = model->l;
        svm_node SV[37];
        svm_parameter _param = model->param;
        float sv_coef[l];
      
        float kvalue[l];
        
        #pragma omp parallel default(none) num_threads(CORE) shared(sv_coef,data_model, kvalue, x, l, _param) private(j, SV)
        {
                #pragma omp barrier
                #pragma omp for nowait 
                
                for(i=0; i < l; i++){
                        
                        sv_coef[i]=data_model[i][0]; 
                        
                        for(j=1;j<=36;j++){
                                SV[j-1].index = j;
                                SV[j-1].value = data_model[i][j]; 
                        }
                        SV[36].index = -1;
                        SV[36].value = 0.0f;
                        
                        kvalue[i] = kernel_function(x, SV,_param); 
                     
                }
                
       
        }//omp
        
        
        int *start = malloc(sizeof(int)*nr_class);
        start[0] = 0;
        for(i=1;i<nr_class;i++)
                start[i] = start[i-1]+model->nSV[i-1];
        
        int *vote = malloc(sizeof(int)*nr_class);
        for(i=0;i<nr_class;i++)
                vote[i] = 0;
        
        int p=0;
        //      int j=0;
        float sum = 0.0f;
    
        for(i=0;i<nr_class;i++)
                for(j=i+1;j<nr_class;j++)
                {
                        sum = 0.0f;
                        int si = start[i];
                        int sj = start[j];
                        int ci = model->nSV[i];
                        int cj = model->nSV[j];
                        
                        int k;
                        float *coef1 = &sv_coef[j-1];
                        float *coef2 = &sv_coef[i];
                        
                        for(k=0;k<ci;k++){
                                sum += coef1[si+k] * kvalue[si+k];                        
                        }
                        for(k=0;k<cj;k++)
                                sum += coef2[sj+k] * kvalue[sj+k];
                        
                        sum -= model->rho[p]; 
                        dec_values[p] = sum;  
                        
                        if(dec_values[p] > 0.0f)
                                ++vote[i];
                        else
                                ++vote[j];
                        p++;
                        
                        
                }
                
                int vote_max_idx = 0;
                for(i=1;i<nr_class;i++)
                        if(vote[i] > vote[vote_max_idx])
                                vote_max_idx = i;
                        
                        printf("\nRESULT:%d\n",model->label[vote_max_idx]);
                
                return 0;
}


