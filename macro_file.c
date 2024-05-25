#include <stdlib.h>
#include <stdio.h>

int GCD(int x, int y){
    for(x,y; y!=0;){
        int temp = x;
        x = y;
        y = temp % y;
    } 
    return x;
}




