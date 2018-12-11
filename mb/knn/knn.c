#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "oprecomp.h"

FILE *open_data(const char *file, int *rows, int *columns)
{
    FILE *data = fopen(file, "r");
    if (data == NULL) {
        perror(file);
        exit(1);
    }

    // read data size
    if (fscanf(data, "%d %d\n", rows, columns) != 2) {
        perror("Error reading header");
        exit(1);
    }
    return data;
}

char ***read_category(FILE *data, int columns)
{
    char buffer[1024];
    char ***cat = malloc(sizeof(char **) * columns);
    for (int i = 0; i < columns; i++) {
        if (fgets(buffer, sizeof(buffer), data) == NULL) {
            perror("Error reading categories");
            exit(1);
        }
        char *t = strtok(buffer, ",\n");
        if (t == NULL) {
            fprintf(stderr, "Error reading categories\n");
            exit(1);
        }
        int n = atoi(t);
        if (n <= 0) cat[i] = NULL;
        else {
            cat[i] = malloc(sizeof(char *) * n);
            for (int j = 0; j < n; j++) {
                t = strtok(NULL, ",\n");
                if (t == NULL) {
                    fprintf(stderr, "Error reading categories\n");
                    exit(1);
                }
                cat[i][j] = strcpy(malloc((strlen(t) + 1) * sizeof(char)), t);
            }
        }
    }
    return cat;
}

double *read_data(FILE *data, int rows, int columns)
{
    double *x = malloc(rows * columns * sizeof(double));
    if (x == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            if (fscanf(data, j == 0 ? "%lf" : ",%lf", x + i * columns + j) != 1) {
                fprintf(stderr, "Error reading data row %d column %d\n", i, j);
                exit(1);
            }
        }
    }
    return x;
}

struct aux {
    double dist;
    int index;
};

int compar(const void *a, const void *b)
{
    double c = ((struct aux *)a)->dist;
    double d = ((struct aux *)b)->dist;
    if (c < d) return -1;
    else if (c > d) return 1;
    else return 0;
}

int vote(int K, double *x, int rows, int columns, double *data, struct aux *v)
{
    for (int i = 0; i < rows; i++) {
        double a = 0.0;
        for (int j = 0; j < columns; j++) {
            double b = (data[i * columns + j] - x[j]);
            a += b * b;
        }
        v[i].dist = a;
        v[i].index = i;
    }
    qsort(v, rows, sizeof(struct aux), compar);
    int c[1]; c[0] = 0; c[1] = 0;
    for (int i = 1; i < K + 1; i++) { // first element is himself
        // ballot (assumes binary clasification)
        if (data[v[i].index * columns + columns - 1] == 0) c[0]++;
        else c[1]++;
        // c[b]++; does not with optimization GCC bug}
    }
    if (c[0] >= c[1]) return 0;
    else return 1;
}

int main(int argc, const char* argv[])
{
    int K, rows, columns, samples;
    if (argc != 4 || (K = atoi(argv[1])) <= 0 ||
            (samples = atoi(argv[3])) <= 0) {
        fprintf(stderr, "usage: <k> <data file> <samples>\n");
        return 1;
    }
    FILE *f = open_data(argv[2], &rows, &columns);

    read_category(f, columns);
    double *x = read_data(f, rows, columns);
    fclose(f);

    struct aux *aux = malloc(rows * sizeof(struct aux));
    if (aux == NULL) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    int TP = 0, TN = 0, FP = 0, FN = 0;

    printf("# K: %d\n", K);
    printf("# datafile: %s\n", argv[2]);
    printf("# problem_size: %d\n", rows);
    printf("# dimension: %d\n", columns);
    printf("# samples: %d\n", samples);
    oprecomp_start();
    do {

    for (int k = 0; k < samples; k++) {
        int r = vote(K, x + k * columns, rows, columns, x, aux); // ballot result
        int c = x[k * columns + columns - 1]; // true value
        if (r == c) {
            if (r == 0) TN++;
            else TP++;
        } else {
            if (r == 0) FN++;
            else FP++;
        }
    }

    } while (oprecomp_iterate());
    oprecomp_stop();
    printf("# TP: %d\n", TP);
    printf("# TN: %d\n", TN);
    printf("# FP: %d\n", FP);
    printf("# FN: %d\n", FN);
    printf("# Sensitiviy:  %f\n", (double)TP / (TP + FN));
    printf("# Specificity: %f\n", (double)TN / (TN + FP));

    return 0;
}
