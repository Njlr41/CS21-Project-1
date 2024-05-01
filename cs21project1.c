#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define base 0x10004000 // base address for symbol table

typedef struct node Symbol;

struct node{
  char name[100];
  Symbol *next;
  int address;
};

int needs_target(char mnem[]);
void append_symbol(char symbol_name[], int address_val, Symbol **head);
int symbol_exists(char symbol_name[], Symbol *head);
void update_address(char symbol_name[], int diff, Symbol *head);

int main()
{
  char *file_name = "sample_input.txt"; // file name
  FILE *fp = fopen(file_name, "r");     // read file
  FILE *output = fopen("output.txt", "w"); // write file

  if(fp == NULL){
    printf("Error");
    return 1;
  }

  char symbol[100], mnemonic[100], c;
  int number_of_instructions;
  int line_number = 1;
  Symbol *head = NULL;

  fscanf(fp, "%d", &number_of_instructions);

  for(int i = 0; i < number_of_instructions; i++){
    fscanf(fp,"%s%c", &symbol, &c);
    printf("\n%s (%d)", symbol, (int)c);

    if(symbol[strlen(symbol)-1] == ':'){ // label
      // remove ':' in symbol
      symbol[strlen(symbol)-1] = '\0';

      // add symbol to the symbol table
      if(symbol_exists(symbol,head))
        // symbol was already encountered
        // update address
        // update_address(symbol,10,head);
        update_address(symbol,(line_number-1),&head);

      else
        // first encounter of symbol
        append_symbol(symbol,base+(line_number-1)*4,&head);

      printf("=>label");
      }

    else if(c == '\n'){ // potential label
      // check if mnemonic needs a target/label
      if(needs_target(mnemonic)){
        // mnemonic needs a target/label
        // add symbol to symbol table IF it doesn't exist yet
        if(symbol_exists(symbol,head))
          append_symbol(symbol,0,&head);

        printf("=>label");
      }
      
      else printf("=>operand"); // immediate/register operand
    }

    else if(symbol[strlen(symbol)-1] != ','){ // mnemonic
      //symbol[strlen(symbol)-1] = '\0';
      strcpy(mnemonic, symbol);
      printf("=>mnemonic");
    }

    else printf("=>operand"); // operand

    if(c == '\n') printf("\n[Line %d]",++line_number);
  }

  printf("\n\n");

  // print symbol table
  Symbol *temp = head;
  while(temp){
    printf("[%5s 0x%X]",temp->name, temp->address);
    fprintf(output, "[%5s 0x%X]",temp->name, temp->address); // write output to output.txt
    if(temp->next) printf("\n");
    if(temp->next) fprintf(output ,"\n"); // write output to output.txt
    temp = temp->next;
  }
  return 0;
}

int needs_target(char mnem[]){
  // check if mnemonic of instruction associated with operand needs target
  char lst[6][5] = {"beq", "bne", "lw", "sw", "j", "jal"};
  for(int i=0; i<6; i++)
    if(strcmp(lst[i],mnem)==0) return 1;
  return 0;
}

void append_symbol(char symbol_name[], int address_val, Symbol **head){
  // create new symbol for label
  Symbol *new_symbol = (Symbol *)malloc(sizeof(Symbol));
  strcpy(new_symbol->name, symbol_name);
  new_symbol->next = NULL;
  new_symbol->address = address_val;

  // add symbol at the end of the linked list
  if(*head == NULL)
    // empty symbol table
    *head = new_symbol;
  else{
    // append symbol at the end of the table
    Symbol *temp = *head;
    while(temp->next) temp = temp->next;
    temp->next = new_symbol;
  }
}

void update_address(char symbol_name[], int diff, Symbol *head){
  // update address of symbol that was already encountered
  Symbol *temp = head;
  while(1){
    if(strcmp(symbol_name,temp->name)==0){
      temp->address = base+diff*4;
      return;
    }
  }
}

int symbol_exists(char symbol_name[], Symbol *head){
  // check if symbol is already in the symbol table
  Symbol *temp = head;
  while(temp){
    if(strcmp(symbol_name,temp->name)==0) return 1;
    return 0;
  }
}