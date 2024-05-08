#include <stdlib.h>
#include <stdio.h>

int GCD(int x, int y){
  for(x,y; y!=0;){
   int temp = x;
   x = y;
   y = temp%y;
  } 
  return x;
};
void print_str(char* string){
  printf("%s", string);
}
void read_str(){

} // Ask Ma'am regarding parameters
void print_integer(int number){
  printf("%d", number);
}
void read_integer(int number){
  scanf("%d", &number);
}
void exit();




