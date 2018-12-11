#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <math.h>

#include "libSVM_predict.h"

#include "utils.h"

PULP_L1_DATA extern struct svm_model *model;
PULP_L1_DATA struct svm_node **predict_data;

PULP_L1_DATA int sample_count = 0;
PULP_L1_DATA int N_smaple = 1;


// load model from handcoded values...
void svm_load_model()
{
        int i = 0;
        int j=0;
        model = (struct svm_model*) malloc(sizeof(struct svm_model));
        
        model->param.svm_type = 0;
        model->param.kernel_type = 2;
        model->param.degree = 0;
        model->param.gamma = 0.0277778;
        model->param.coef0 = 0;
        model->nr_class = 2;
        model->l = 315;
        
        int n = model->nr_class * (model->nr_class-1)/2;
        model->rho = (float *) malloc(sizeof(float)*n);
        model->rho[0] = 0.138475;
        
        
        model->label = (int*)malloc(sizeof(int)*model->nr_class);
        model->label[0] = 0;
        model->label[1] = 1;
        
        
        model->nSV = (int*)malloc(sizeof(int)*model->nr_class);
        model->nSV[0] = 161;
        model->nSV[1] = 154;
        
        int m = model->nr_class - 1;
        int l = model->l;
        model->sv_coef = (float**) malloc(sizeof(float)*m);
        model->SV = (svm_node**) malloc(sizeof(svm_node)*l);
}