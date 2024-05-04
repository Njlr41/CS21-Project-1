#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BASE_TEXT 0x00400000 // base address for the text segment
#define BASE_DATA 0x10000000 // base address for the data segment
typedef struct node Symbol;

struct node{
  char name[100];
  char str_value[100];
  int int_value;
  Symbol *next;
  int address;
};

typedef struct instruction Instruction;

struct instruction{
  char mnemonic[100];
  char type;
  int target;
  int rs;
  int rt;
  int rd;
  int immediate;
};

int needs_target(char mnem[]);
void append_symbol(char symbol_name[], int address_val, Symbol **head);
int symbol_exists(char symbol_name[], Symbol *head);
void update_address(char symbol_name[], int diff, int in_data, Symbol *head);
Instruction create_instruction(char* mnemonic, char type, int target, int rs, int rt, int rd, int immediate);
void update_str(char str_value[], Symbol *head);
void update_int(int int_value, Symbol *head);

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
  int flag = 1, in_data_segment = 0;
  int byte_counter = 0;
  Symbol *head = NULL;
  Instruction *temp_instruction = (Instruction *)malloc(sizeof(Instruction));

  fscanf(fp, "%d", &number_of_instructions);
  Instruction InstructionList[number_of_instructions + 1];
  while(line_number <= number_of_instructions){
    fscanf(fp, "%s%c", &symbol, &c);
    printf("\n%s (%d)", symbol, (int)c);

    if(strcmp(symbol,".data:")==0){
      // already reached the data segment
      // update in_data_segment to 1
      in_data_segment = 1;
      printf("=> data segment");
    }
    else if(in_data_segment==1){ // DATA SEGMENT
      if(symbol[strlen(symbol)-1] == ':'){ // label
        // remove ':' at the end of the symbol
        symbol[strlen(symbol)-1] = '\0';

        // add symbol to the symbol table
        if(symbol_exists(symbol, head))
          // symbol was already encountered
          // update address
          // update_address(symbol,10,head);
          update_address(symbol, byte_counter, 1, head);

        else
          // first encounter of symbol
          append_symbol(symbol, BASE_DATA + byte_counter, &head);
      }
      else if(symbol[0] == '.'){ // either .ascii or .word
        strcpy(symbol, mnemonic);
        printf("=> allocation");
      }
      else{ // either allocate string, allocate bytes, or operand of .ascii and .word
        if(strcmp(mnemonic,".ascii")==0){ // allocate string
          // update value of corresponding symbol in the table
          // update byte counter by length of the string (+1 for terminator \0)
          symbol[strlen(symbol)-1] = '\0';  // remove closing "
          update_str(symbol+1, head);       // remove opening "
          byte_counter += (strlen(symbol)+1);
          printf("=> string");
        }
        else if(strcmp(symbol,".word")==0){ // allocate integer
          // update value of corresponding symbol in the table
          // update byte counter by 4 (size of integer is 4 bytes)
          update_int(atoi(symbol), head);
          byte_counter += 4;
          printf("=> int");
        }
        else{
          /*
          Allocation of string:
          a l l o c a t e _ s t  r  (  <label_name>,"<str_value>\0")
          0|1|2|3|4|5|6|7|8|9|10|11|12|13|14
                                       ^ starting index

          Allocation of integer:
          a l l o c a t e _ b y  t  e  s  (  <labe_name>,<int_value>)
          0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15
                                             ^ starting index
          */
          printf("=> macro");
          if(symbol[9]=='s'){ // allocate string
            // initialize variables for
            // label name, string value,
            // and starting indices
            char label_name[100] = {}, str_value[100] = {};
            int i=13, j=0;

            // obtain label name and string value
            for(; symbol[i]!=','; i++, j++)
              label_name[j] = symbol[i];

            for(i=i+2,j=0;symbol[i]!='"';i++,j++)
              str_value[j] = symbol[i];

            // add symbol to symbol table
            // update value of corresponding symbol in the table
            // update byte counter by length of the string (+1 for terminator \0)
            append_symbol(label_name, BASE_DATA + byte_counter*4, &head);
            update_str(str_value, head);
            byte_counter += (strlen(symbol)+1);
          }
          else{ // allocate integer
            // initialize variables for
            // label name, integer value,
            // and starting indices
            char label_name[100] = {}, int_value[100] = {};
            int i=15, j=0;

            // obtain label name and integer value
            for(;symbol[i]!=','; i++, j++)
              label_name[j] = symbol[i];
            for(i=i+1,j=0;symbol[i]!=')';i++,j++)
              int_value[j] = symbol[i];

            // add symbol to symbol table
            // update value of corresponding symbol in the table
            // update byte counter by 4 (size of integer is 4 bytes)
            append_symbol(label_name, BASE_DATA + byte_counter*4, &head);
            update_int(atoi(int_value), head);
            byte_counter++;
          }
        }
      }
    }
    else{ // TEXT SEGMENT
      if(symbol[strlen(symbol)-1] == ':'){ // label
        // remove ':' in symbol
        symbol[strlen(symbol)-1] = '\0';

        // add symbol to the symbol table
        if(symbol_exists(symbol, head))
          // symbol was already encountered
          // update address
          // update_address(symbol,10,head);
          update_address(symbol, (line_number - 1), 0, head);

        else
          // first encounter of symbol
          append_symbol(symbol, BASE_TEXT + (line_number - 1)*4, &head);

        printf("=>label");
        }

      else if(c == '\n'){ // potential label
        // check if mnemonic needs a target/label
        if(needs_target(mnemonic)){
          // mnemonic needs a target/label
          // add symbol to symbol table IF it doesn't exist yet
          if(symbol_exists(symbol, head))
            append_symbol(symbol, 0, &head);

          printf("=>label");
        }
        
        else printf("=>operand"); // immediate/register operand
      }

      else if(symbol[strlen(symbol) - 1] != ','){ // mnemonic
        //symbol[strlen(symbol)-1] = '\0';
        strcpy(mnemonic, symbol);
        strcpy(temp_instruction->mnemonic, symbol);
        printf("=>mnemonic");
      }

      else printf("=>operand"); // operand
    }

    if(c == '\n' || number_of_instructions == line_number){
      InstructionList[line_number - 1] = create_instruction(temp_instruction->mnemonic, temp_instruction->type, temp_instruction->target, temp_instruction->rs, temp_instruction->rt, temp_instruction->rd, temp_instruction->immediate);
      printf("\n[Line %d]", ++line_number);
      // reset
      strcpy(mnemonic,"\0");
    }
      
  }

  printf("\n\n");

  for(int i = 0; i < number_of_instructions; i++)
  {
    printf("%s\n", (InstructionList[i]).mnemonic);
  }
  // print symbol table
  Symbol *temp = head;
  while(temp){
    printf("[%5s 0x%X]", temp->name, temp->address);
    fprintf(output, "[%5s 0x%X]", temp->name, temp->address); // write output to output.txt
    if(temp->next) printf("\n");
    if(temp->next) fprintf(output,"\n"); // write output to output.txt
    temp = temp->next;
  }
  return 0;
}

Instruction create_instruction(char* mnemonic, char type, int target, int rs, int rt, int rd, int immediate){
  Instruction *A = (Instruction *)malloc(sizeof(Instruction));
  strcpy(A->mnemonic, mnemonic);
  A->type = type;
  A->target = target;
  A->rs = rs;
  A->rt = rt;
  A->rd = rd;
  A->immediate = immediate;
  return *A;
}
int needs_target(char mnem[]){
  // check if mnemonic of instruction associated with operand needs target
  char lst[6][5] = {"beq", "bne", "lw", "sw", "j", "jal"};
  for(int i=0; i<6; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
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

void update_str(char str_value[], Symbol *head){
  // update str_value field of symbol in data segment
  // => the last appended symbol in the table corresponds to
  // => the symbol to be updated
  Symbol *temp = head;
  while(temp->next != NULL) temp = temp->next;
  strcpy(temp->str_value,str_value);
  return;
}

void update_int(int int_value, Symbol *head){
  // update str_value field of symbol in data segment
  // => the last appended symbol in the table corresponds to
  // => the symbol to be updated
  Symbol *temp = head;
  while(temp->next != NULL) temp = temp->next;
  temp->int_value, int_value;
  return;
}

void update_address(char symbol_name[], int diff, int in_data, Symbol *head){
  // update address of symbol that was already encountered
  Symbol *temp = head;
  while(1){
    if(strcmp(symbol_name, temp->name)==0){
      temp->address = in_data ? BASE_DATA + diff : BASE_TEXT + diff*4;
      return;
    }
    temp = temp->next;
  }
}

int symbol_exists(char symbol_name[], Symbol *head){
  // check if symbol is already in the symbol table
  Symbol *temp = head;
  while(temp){
    if(strcmp(symbol_name,temp->name)==0) return 1;
    temp = temp->next;
  }
  return 0;
}