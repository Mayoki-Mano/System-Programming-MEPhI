#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

char *readline() {
    char buf[81] = {0};
    char *res = NULL;
    int len = 0;
    int n;
    do {
        n = scanf("%80[^\n]", buf);
        if (n < 0) {
            if (!res) {
                return NULL;
            }

        } else if (n > 0) {
            unsigned buf_len = strlen(buf);
            unsigned str_len = len + buf_len;
            char * tmp = res;
            res = (char *) realloc(res, sizeof(char) * (str_len + 1));
            if (!res){
                if (!tmp)
                    free(tmp);
                return NULL;
            }
            for (int i = 0; i < buf_len; i++) {
                res[len] = buf[i];
                len++;
            }
        } else {
            scanf("%*c");
        }
    } while (n > 0);
    if (len > 0) {
        if (!res)
            return NULL;
        res[len] = '\0';
    } else {
        res = (char *) calloc(1, sizeof(char));
    }
    return res;
}

void first_task() {
    char second_name[50];
    char first_name[50];
    printf("Enter your name:\n");
    scanf("%49s", first_name);
    printf("Enter your second name:\n");
    scanf("%49s", second_name);
    printf("My name %s %s   %d %d %0.3f %0.3f %d\n", first_name,second_name,789,-76,956.361,-395.659,794);
}

void second_task() {
    int a,b,c,d;
    int min = INT_MAX;
    printf("Enter a,b,c,d:\n");
    scanf("%d%d%d%d",&a,&b,&c,&d);
    if (abs(a%2)==1 && a<min)
        min = a;
    if (abs(b%2)==1 && b<min)
        min = b;

    if (abs(c%2)==1 && c<min)
        min = c;

    if (abs(d%2)==1 && d<min)
        min = d;

    if (min==INT_MAX){
        printf("There are no odd negative numbers among the numbers entered.");
    }else{
        printf("Minimum odd negative number: %d",min);
    }
}

void third_task() {
    int A[20]={1,2,3,4,-5,-6,-8,-9,-10,11,12,13,14,15,16,17,18,19,-20,21};
    printf("Not negative numbers: \n");
    for (int i=0; i<20;++i){
        if (A[i]>=0){
            printf("%d ",A[i]);
        }
    }
    printf("\nNegative numbers: \n");
    for (int i=0; i<20;++i){
        if (A[i]<0){
            printf("%d ",A[i]);
        }
    }
    int M[10][10]={{1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
                   {1,2,0,0,0,0,4,8,9,10},
    };
    for (int i=0; i<10;++i){
        float avg=0;
        for (int j=0; j<10;++j)
            avg+=(float)M[i][j];
        avg/=10;
        printf("\nAvg %d row: %0.2f",i,avg);
    }

}



void four_task() {
    double numerator_0 = 1, denominator_0 = 1;
    double numerator_1 = 2, denominator_1 = 1;

    double a_prev = numerator_0 / denominator_0;
    double a_curr = numerator_1 / denominator_1;

    int n = 1;

    while (fabs(a_curr - a_prev) >= 0.001) {
        double new_numerator = numerator_1 + numerator_0;
        double new_denominator = denominator_1 + denominator_0;

        numerator_0 = numerator_1;
        denominator_0 = denominator_1;
        numerator_1 = new_numerator;
        denominator_1 = new_denominator;
        a_prev = a_curr;
        a_curr = new_numerator / new_denominator;
        n++;
    }
    printf("The sequence member with index is found, n = %d\n", n);
    printf("a[n-1] = %.6f\n", a_prev);
    printf("a[n] = %.6f\n", a_curr);
    printf("|a[n] - a[n-1]| = %.6f\n", fabs(a_curr - a_prev));
}

void fifth_task(){
    printf("Enter string:\n");
    char * text = readline();
    char *text_tmp = text;
    const char *start, *end;
    printf("Words ending with r:\n");
    while (*text != '\0') {
        while (*text != '\0' && !isalnum(*text)) {
            text++;
        }
        start = text;
        while (*text != '\0' && isalnum(*text)) {
            text++;
        }
        end = text - 1;
        if (start <= end && *end == 'r') {
            while (start <= end) {
                printf("%c",*start++);
            }
            printf("\n");
        }
    }
    free(text_tmp);
}

int main() {
//    first_task();
//    second_task();
//    third_task();
//    four_task();
//    fifth_task();
    return 0;
}