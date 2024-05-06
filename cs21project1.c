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
  char target[100];
  int rs;
  int rt;
  int rd;
  int immediate;
};

int NEEDS_TARGET(char mnem[]);
void APPEND_SYMBOL(char symbol_name[], int address_val, Symbol **head);
int SYMBOL_EXISTS(char symbol_name[], Symbol *head);
void UPDATE_ADDRESS(char symbol_name[], int diff, int in_data, Symbol *head);
Instruction CREATE_INSTRUCTION(Instruction *temp);
void UPDATE_STR(char str_value[], Symbol *head);
void UPDATE_INT(int int_value, Symbol *head);
int REG_NUMBER(char reg_name[]);

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
  printf("[Line 1]");
  while(fscanf(fp, "%s%c", &symbol, &c)!=EOF){
    printf("\n%30s ", symbol);
    if(strcmp(symbol,".data")==0){
      /*
      FIRST PASS already reached the DATA segment
      => update in_data_segment to 1
      */
      in_data_segment = 1;
      printf("=> data segment");
    }
    else if(in_data_segment==1){ // DATA SEGMENT
      if(symbol[strlen(symbol)-1] == ':' && strcmp(mnemonic,"allocate_str") != 0 && strcmp(mnemonic,".asciiz") != 0){ // label
        // remove ':' at the end of the symbol
        symbol[strlen(symbol)-1] = '\0';

        // add symbol to the symbol table
        if(SYMBOL_EXISTS(symbol, head))
          // symbol was already encountered
          // update address
          UPDATE_ADDRESS(symbol, byte_counter, 1, head);

        else
          // first encounter of symbol
          // offset (byte_counter) is already in bytes
          APPEND_SYMBOL(symbol, BASE_DATA + byte_counter, &head);
        printf("=> label");
      }
      else if(symbol[0] == '.'){ // either .asciiz or .word
        strcpy(mnemonic,symbol);
        printf("=> allocation (%s)",mnemonic);
      }
      else{ // either allocate string, allocate bytes, or operand of .asciiz and .word
        if(strcmp(mnemonic,".asciiz")==0 || strcmp(mnemonic,"allocate_str")==0){ // allocate string
          if(symbol[strlen(symbol)-1] == '"'){
            /*
            symbol is operand of .asciiz
            => update byte_counter by length of string without quotation marks
            => remove " at the end
            */ 
            byte_counter += (strlen(symbol)-1);   // (length-2)+1
            symbol[strlen(symbol)-1] = '\0';    // remove closing "
          }
          else if(symbol[strlen(symbol)-1] == ')'){
            /*
            symbol is the last concatenation of allocate_str
            => update byte_counter by length of string without ")
            => remove ") at the end
            */ 
            byte_counter += (strlen(symbol)-1); // (length-2)+1
            symbol[strlen(symbol)-1] = '\0';    // remove closing )
            symbol[strlen(symbol)-1] = '\0';    // remove closing "
          }
          else {
            /*
            symbol is concatenation of allocate_str
            => update byte_counter by length of string without "
            => remove " at the end
            */ 
            byte_counter += (strlen(symbol)+1);
          }
          
          // update str_value of corresponding symbol
          if(symbol[0] == '"')
            UPDATE_STR(symbol+1, head);         // remove opening "
          else
            UPDATE_STR(symbol, head);
          
          printf("=> string");
        }
        else if(strcmp(mnemonic,".word")==0){ 
          /*
          symbol is the integer operand of .word
          => convert string to integer using atoi()
          => increment byte_counter by 4 (size of an integer is 4 bytes)
          */
          UPDATE_INT(atoi(symbol), head);
          byte_counter += 4;
          printf("=> int");
        }
        else{
          /*
          symbol is a macro of string/integer allocation

          Allocation of string:
          a l l o c a t e _ s t  r  (  <label_name>,"<str_value>\0")
          0|1|2|3|4|5|6|7|8|9|10|11|12|13|14
                                       ^ starting index

          Allocation of integer:
          a l l o c a t e _ b y  t  e  s  (  <label_name>,<int_value>)
          0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15
                                             ^ starting index
          */
          printf("=> macro");
          if(symbol[9]=='s'){
            /*
            allocate string
            => initialize variables for label name, string value, and starting indices
            */
            strcpy(mnemonic, "allocate_str");
            char label_name[100] = {}, str_value[100] = {};
            int i=13, j=0;

            /*
            obtain label name and string value
            => label name ends before ,
            => string value can either be one word (ends with ") or multiple words
            */
            for(; symbol[i] != ','; i++, j++)
              label_name[j] = symbol[i];
            for(i=i+2, j=0; symbol[i] != '"' && symbol[i] != '\0'; i++, j++)
              str_value[j] = symbol[i];
            
            /*
            add symbol to symbol table
            => update string value of corresponding symbol in the table
            => update byte counter by length of the string (+1 for the terminator \0)
            */
            APPEND_SYMBOL(label_name, BASE_DATA + byte_counter, &head);
            UPDATE_STR(str_value, head);
            byte_counter += (strlen(str_value)+1);
          }
          else{
            /*
            allocate integer
            => initialize variables for label name, integer value, and starting indices
            */
            char label_name[100] = {}, int_value[100] = {};
            int i=15, j=0;

            /*
            obtain label name and string value
            => label name ends before ,
            => integer value ends before )
            */
            for(; symbol[i]!=','; i++, j++)
              label_name[j] = symbol[i];
            for(i=i+1,j=0;symbol[i]!=')';i++,j++)
              int_value[j] = symbol[i];

            /*
            add symbol to symbol table
            => update integer value of corresponding symbol in the table
            => increment byte counter by 4 (size of an integer is 4 bytes)
            */
            APPEND_SYMBOL(label_name, BASE_DATA + byte_counter, &head);
            UPDATE_INT(atoi(int_value), head);
            byte_counter+=4;
          }
        }
      }
    }
    else{ // TEXT SEGMENT
      if(symbol[strlen(symbol)-1] == ':'){ // label
        // remove ':' in symbol
        symbol[strlen(symbol)-1] = '\0';

        // add symbol to the symbol table
        if(SYMBOL_EXISTS(symbol, head))
          // symbol was already encountered
          // update address
          // UPDATE_ADDRESS(symbol,10,head);
          UPDATE_ADDRESS(symbol, (line_number - 1), 0, head);

        else
          // first encounter of symbol
          APPEND_SYMBOL(symbol, BASE_TEXT + (line_number - 1)*4, &head);

        printf("=> label");
        }

      /* else if(c == '\n'){ // potential label or syscall
        if(strcmp(symbol,"syscall")==0){
          strcpy(mnemonic,symbol);
          printf("=> syscall");
        }
        // check if mnemonic needs a target/label
        else if(NEEDS_TARGET(mnemonic)){
          // mnemonic needs a target/label
          // add symbol to symbol table IF it doesn't exist yet
          printf("%d",SYMBOL_EXISTS(symbol,head));
          if(!SYMBOL_EXISTS(symbol, head))
            APPEND_SYMBOL(symbol, 0, &head);

          printf("=> label");
        }
        else printf("=> operand"); // immediate/register operand
      } */

      else if(c != '\n'){ // mnemonic
        strcpy(mnemonic, symbol);
        strcpy(temp_instruction->mnemonic, symbol);
        printf("=> mnemonic");
      }

      else {

        if(IS_RTYPE(mnemonic)){
          if(strcmp(mnemonic,"jr")==0){ // store to rs
            temp_instruction->rs = REG_NUMBER(symbol);
          }
          
          else if(strcmp(mnemonic,"move")==0){ // store to rd->rs
            char operand[5] = {};
            int i = 0, j = 0;

            for(; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rd = REG_NUMBER(operand);
            temp_instruction->rs = REG_NUMBER(symbol+(i+1));
          }

          else{ // store to rd->rs->rt
            char operand[5] = {};
            int i = 0, j = 0;

            for(; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rd = REG_NUMBER(operand);
            
            strcpy(operand,"\0"); // reset operand

            for(i=i+1, j=0; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rs = REG_NUMBER(operand);
            temp_instruction->rt = REG_NUMBER(symbol+(i+1));
          }
          printf("=> operand"); // operand
        }

        else if(IS_ITYPE(mnemonic)){
          // lui, lw, sw
          if(mnemonic[0] == 'l' || mnemonic[1] == 'w'){ // store to rt,imm
             char operand[5] = {};
              int i = 0, j = 0;

              for(; symbol[i]!=','; i++, j++)
                operand[j] = symbol[i];
              temp_instruction->rt = REG_NUMBER(operand);

              strcpy(operand,"\0"); // reset reg_name

              for(i=i+1, j=0; symbol[i]!='\0' || symbol[i]!='('; i++, j++)
                operand[j] = symbol[i];
              temp_instruction->immediate = atoi(operand);

              // lw, sw
              if(mnemonic[1] == 'w'){
                strcpy(operand,"\0");
                for(i=i+1, j=0; symbol[i]!=')'; i++, j++)
                  operand[j] = symbol[i];
                temp_instruction->rs = REG_NUMBER(operand);
              }
          }
          
          // beq, bne
          else if(mnemonic[0]=='b'){ // store to rs,rt,target
            char operand[5] = {};
            int i = 0, j = 0;

            for(; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rs = REG_NUMBER(operand);
            
            strcpy(operand,"\0"); // reset operand

            for(i=i+1, j=0; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rt = REG_NUMBER(operand);
            strcpy(temp_instruction->target,symbol+(i+1));
          } 

          // addi, addiu
          else{ // store to rt,rs,imm
            char operand[5] = {};
            int i = 0, j = 0;

            for(; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rt = REG_NUMBER(operand);
            
            strcpy(operand,"\0"); // reset operand

            for(i=i+1, j=0; symbol[i]!=','; i++, j++)
              operand[j] = symbol[i];
            temp_instruction->rs = REG_NUMBER(operand);
            temp_instruction->immediate = atoi(symbol+(i+1));
          }
          printf("=> operand"); // operand
        }

        else{
          if(!SYMBOL_EXISTS(symbol, head))
            APPEND_SYMBOL(symbol, 0, &head);
          strcpy(temp_instruction->target, symbol);
          printf("=> label");
        }
        
      }
    }

    if(c == '\n'){
      InstructionList[line_number - 1] = CREATE_INSTRUCTION(temp_instruction->mnemonic);
      // reset
      strcpy(mnemonic,"\0");
      free(temp_instruction);
      temp_instruction = (Instruction *)malloc(sizeof(Instruction));
      // line number
      printf("\n[Line %d]", ++line_number);
      
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
    printf("[%-5s 0x%08X]", temp->name, temp->address);
    fprintf(output, "[%-5s 0x%08X]", temp->name, temp->address); // write output to output.txt
    if(temp->next) printf("\n");
    if(temp->next) fprintf(output,"\n"); // write output to output.txt
    temp = temp->next;
  }

  // print data segment symbols
  temp = head;
  int printed=0;
  while(temp){
    if(strlen(temp->str_value)!=0){
      if(printed==0){
        printf("| %-5s | %-20s | %10s |\n", "Label", "Content", "Address");
        for(int k=0; k<45; k++) printf("-");
        printf("\n");
        printed=1;
      }
      printf("| %-5s | %-20s | 0x%08X |", temp->name, temp->str_value, temp->address);
    }
    else if(temp->address >= BASE_DATA){
      if(printed==0){
        printf("| %-5s | %-20s | %10s |\n", "Label", "Content", "Address");
        for(int k=0; k<45; k++) printf("-");
        printf("\n");
        printed=1;
      }
      printf("| %-5s | %-20d | 0x%08X |", temp->name, temp->int_value, temp->address);
    }
    if(temp->next) printf("\n");
    temp = temp->next;
  }
  return 0;
}

Instruction CREATE_INSTRUCTION(Instruction *temp){
  Instruction *A = (Instruction *)malloc(sizeof(Instruction));
  strcpy(A->mnemonic, temp->mnemonic);
  strcpy(A->target, temp->target);
  A->rs = temp->rs;
  A->rt = temp->rt;
  A->rd = temp->rd;
  A->immediate = temp->immediate;
  return *A;
}

int NEEDS_TARGET(char mnem[]){
  // check if mnemonic of instruction associated with operand needs target
  char lst[6][5] = {"beq", "bne", "lw", "sw", "j", "jal"};
  for(int i=0; i<6; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
  return 0;
}

int IS_RTYPE(char mnem[]){
  // check if mnemonic of instruction is R-Type
  char lst[7][5] = {"add","sub","and","or","slt","move","jr"};
  for(int i=0; i<7; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
  return 0;
}

int IS_ITYPE(char mnem[]){
  // check if mnemonic of instruction is I-Type
  char lst[7][6] = {"addi","addiu","lui","lw","sw","beq","bne"};
  for(int i=0; i<7; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
  return 0;
}

int IS_JTYPE(char mnem[]){
  // check if mnemonic of instruction is I-Type
  char lst[2][6] = {"j","jal"};
  for(int i=0; i<2; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
  return 0;
}

void APPEND_SYMBOL(char symbol_name[], int address_val, Symbol **head){
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

void UPDATE_STR(char str_value[], Symbol *head){
  /*
  update str_value field of symbol in data segment
  => the last appended symbol in the table corresponds to the symbol to be updated
  */
  Symbol *temp = head;
  while(temp->next != NULL) temp = temp->next;
  if(strlen(temp->str_value) == 0) 
    // new 
    strcpy(temp->str_value,str_value);
  else{ 
    // concatenate string
    strcat(temp->str_value," ");
    strcat(temp->str_value, str_value);
  }

  return;
}

void UPDATE_INT(int int_value, Symbol *head){
  /*
  update str_value field of symbol in data segment
  => the last appended symbol in the table corresponds to the symbol to be updated
  */
  Symbol *temp = head;
  while(temp->next != NULL) temp = temp->next;
  temp->int_value = int_value;
  return;
}

void UPDATE_ADDRESS(char symbol_name[], int diff, int in_data, Symbol *head){
  // update address of symbol that was already encountered
  Symbol *temp = head;
  while(1){
    if(strcmp(symbol_name, temp->name)==0){
      // offset for label in data segment is already in bytes
      temp->address = in_data ? BASE_DATA + diff : BASE_TEXT + diff*4;
      return;
    }
    temp = temp->next;
  }
}

int SYMBOL_EXISTS(char symbol_name[], Symbol *head){
  // check if symbol is already in the symbol table
  Symbol *temp = head;
  while(temp){
    if(strcmp(symbol_name,temp->name)==0) return 1;
    temp = temp->next;
  }
  return 0;
}

int REG_NUMBER(char reg_name[]){
    char num[2] = {reg_name[2],'\0'}; 
    switch(reg_name[1]){
        case 'v':
            return (2 + atoi(num));
        case 'a':
            return (reg_name[2] == 't' ? 1 : 4+atoi(num));
        case 't':
            return (atoi(num) > 7 ? 24 + (atoi(num)-8) : 8 + atoi(num));
        case 's':
            return (reg_name[2] == 'p' ? 29 : 16 + atoi(num));
        case 'k':
            return (26 + atoi(num));
        case 'g':
            return 28;
        case 'f':
            return 30;
        case 'r':
            return 31;
        default:
            return 0;
    }
}