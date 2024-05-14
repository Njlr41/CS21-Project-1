#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "macro_file.c"
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
Instruction *CREATE_INSTRUCTION(Instruction *temp);
void UPDATE_STR(char str_value[], Symbol *head);
void UPDATE_INT(int int_value, Symbol *head);
int REG_NUMBER(char reg_name[]);
Instruction* RESET_TEMP();
void ADD_TO_MEMORY(char MemoryFile[], Symbol *SymbolTable); 
void PRINT_MEMORY(char MemoryFile[], int BYTE_COUNTER);
void PRINT_INSTRUCTIONS(Instruction *InstructionList[], int N_LINES);
void PRINT_DATA_SEGMENT(Symbol *head);
void PRINT_SYMBOL_TABLE(Symbol *head, FILE *output);
int IS_PSEUDO(char mnem[]);
int IS_MACRO(char mnem[]);
int IS_RTYPE(char mnem[]);
int IS_ITYPE(char mnem[]);
int IS_JTYPE(char mnem[]);
Symbol *GET_TAIL(Symbol *head);
char *GET_BINARY(int number);
int HEX_TO_DECIMAL(char* hex);
int GET_SYMBOL_ADDRESS(char label[], FILE* symbol_table);

int main()
{
  FILE *fp = fopen("sample_input.txt", "r");     // read file
  FILE *output = fopen("output.txt", "w+"); // write file
  FILE *machinecode = fopen("machinecode.txt", "w"); // write file

  if(fp == NULL){
    printf("Error");
    return 1;
  }

  // Initialize variables
  char symbol[100], mnemonic[100], c;
  int N_LINES, LINE_NUMBER = 1, IN_DATA_SEGMENT = 0, BYTE_COUNTER = 0;

  // Get NUMBER OF LINES
  fscanf(fp, "%d", &N_LINES);

  // Initialize SYMBOL TABLE
  Symbol *head = NULL;

  // Initialize MEMORY
  char MemoryFile[1000] = {};

  // Initialize TEMP_INSTRUCTION and LIST OF INSTRUCTIONS
  Instruction *temp_instruction = RESET_TEMP();
  Instruction *InstructionList[1000];
  int INST_COUNTER = 0;

  printf("[Line 1]");
  fscanf(fp, "%s%c", &symbol, &c);

  // FIRST PASS
  while(1){
    printf("\n%30s %2d ", symbol, c);
    if(strcmp(symbol,".text")==0){
      /*
      FIRST PASS already reached the TEXT gment
      (1) Update IN_DATA_SEGMENT to 0
      */
      IN_DATA_SEGMENT = 0;
      printf("=> text segment");
    }
    else if(strcmp(symbol,".data")==0){
      /*
      FIRST PASS already reached the DATA segment
      (1) Update IN_DATA_SEGMENT to 1
      */
      IN_DATA_SEGMENT = 1;
      printf("=> data segment");
    }
    else if(IN_DATA_SEGMENT==1){ // DATA SEGMENT
      if(symbol[strlen(symbol)-1] == ':' && strcmp(mnemonic,"allocate_str") != 0 && strcmp(mnemonic,".asciiz") != 0){
        /*
        LABEL Definition
        (1) Remove ':' in symbol
        (2) Check if LABEL was already encountered
        -- LABEL exists in symbol table
        -- LABEL was used in a previous instruction
        -- 2a) YES => Update address of LABEL
        -- 2b) NO => Add LABEL to symbol table
        */
       
        symbol[strlen(symbol)-1] = '\0';
        if(SYMBOL_EXISTS(symbol, head) != -1)
          UPDATE_ADDRESS(symbol, BYTE_COUNTER, 1, head);
        else
          APPEND_SYMBOL(symbol, BASE_DATA + BYTE_COUNTER, &head);
        printf("=> label");
      }
      else if(symbol[0] == '.'){ // either .asciiz or .word
        /*
        NON-MACRO Allocation
        (1) Copy SYMBOL to 'mnemonic'
        */
        strcpy(mnemonic,symbol);
        printf("=> allocation (%s)",mnemonic);
      }
      else{ // either allocate string, allocate bytes, or operand of .asciiz and .word
        if(strcmp(mnemonic,".asciiz")==0 || strcmp(mnemonic,"allocate_str")==0){
          /*
          STRING ALLOCATION
          (1) Check if string is to be allocated using .asciiz or allocate_str
          -- .asciiz  => Remove " if last word in the string
          -- allocate_str => Remove ) if last word in the string
          -- Update BYTE_COUNTER by actual string LENGTH (+1 for \0)
          (2) Update STR_VALUE of LABEL
          -- .asciiz => Remove " if starting word in the string
          */

          if(symbol[strlen(symbol)-1] == '"'){
            BYTE_COUNTER += (strlen(symbol)-1);   // (length-2)+1
            symbol[strlen(symbol)-1] = '\0';      // remove closing "
          }
          else if(symbol[strlen(symbol)-1] == ')'){
            BYTE_COUNTER += (strlen(symbol)-1); // (length-2)+1
            symbol[strlen(symbol)-1] = '\0';    // remove closing )
            symbol[strlen(symbol)-1] = '\0';    // remove closing "
          }
          else {
            BYTE_COUNTER += (strlen(symbol)+1);
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
          INTEGER ALLOCATION
          (1) Convert string to integer using atoi()
          (2) Update INT_VALUE of LABEL
          (3) Increment BYTE_COUNTER by 4 (size of an integer is 4 bytes)
          */
          UPDATE_INT(atoi(symbol), head);
          Symbol *tail = GET_TAIL(head);
          BYTE_COUNTER = BYTE_COUNTER % 4 == 0 ? BYTE_COUNTER : BYTE_COUNTER + (4-(BYTE_COUNTER % 4));
          UPDATE_ADDRESS(tail->name, BYTE_COUNTER, 1, head);
          BYTE_COUNTER+=4;
          printf("=> int");
        }
        else{
          /*
          MACRO ALLOCATION

          Allocation of string:
          a l l o c a t e _ s t  r  (  <label_name>,"<str_value>\0")
          0|1|2|3|4|5|6|7|8|9|10|11|12|13|14
                                       ^ starting index
          (1) Get LABEL_NAME
          (2) Get STR_VALUE
          -- Actual STR_VALUE may be decomposed into multiple words
          (3) Add LABEL to symbol table
          (4) Update STR_VALUE of LABEL
          (5) Update BYTE_COUNTER by the actual string LENGTH (+1 for \0) 

          Allocation of integer:
          a l l o c a t e _ b y  t  e  s  (  <label_name>,<int_value>)
          0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15
                                             ^ starting index
          (1) Get LABEL_NAME
          (2) Get INT_VALUE and convert to integer using atoi()
          (3) Add LABEL to symbol table
          (4) Update INT_VALUE of LABEL
          (5) Update BYTE_COUNTER by 4 (size of integer is 4 bytes) 
          */
          printf("=> macro");
          if(symbol[9]=='s'){
            strcpy(mnemonic, "allocate_str");
            char label_name[100] = {}, str_value[100] = {};
            int i=13, j=0;

            for(; symbol[i] != ','; i++, j++)
              label_name[j] = symbol[i];
            for(i=i+2, j=0; symbol[i] != '"' && symbol[i] != '\0'; i++, j++)
              str_value[j] = symbol[i];
            
            APPEND_SYMBOL(label_name, BASE_DATA + BYTE_COUNTER, &head);
            UPDATE_STR(str_value, head);
            BYTE_COUNTER += (strlen(str_value)+1);
          }
          else{
            char label_name[100] = {}, int_value[100] = {};
            int i=15, j=0;

            for(; symbol[i]!=','; i++, j++)
              label_name[j] = symbol[i];
            for(i=i+1,j=0;symbol[i]!=')';i++,j++)
              int_value[j] = symbol[i];
            
            BYTE_COUNTER = (BYTE_COUNTER % 4) == 0 ? BYTE_COUNTER : BYTE_COUNTER + (4-(BYTE_COUNTER % 4));
            printf("$s");
            APPEND_SYMBOL(label_name, BASE_DATA + BYTE_COUNTER, &head);
            UPDATE_INT(atoi(int_value), head);
            BYTE_COUNTER += 4;
          }
        }
      }
    }
    else{ // TEXT SEGMENT
      if(symbol[strlen(symbol)-1] == ':'){
        /*
        LABEL Definition
        (1) Remove ':' in symbol
        (2) Check if LABEL was already encountered
        -- LABEL exists in symbol table
        -- LABEL was used in a previous instruction
        -- 2a) YES => Update address of LABEL
        -- 2b) NO => Add LABEL to symbol table
        */
        
        symbol[strlen(symbol)-1] = '\0';
        if(SYMBOL_EXISTS(symbol, head) != -1) UPDATE_ADDRESS(symbol, INST_COUNTER, 0, head);
        else APPEND_SYMBOL(symbol, BASE_TEXT + (INST_COUNTER*4), &head);

        printf("=> label");
      }

      else if(c != '\n' && strcmp(mnemonic,"\0")==0){ 
        /*
        MNEMONIC Definition
        (1) Store the MNEMONIC in a temporary variable 'mnemonic'
        -- This will be used for comparison in the latter scans
        -- Differentiates instructions from .data labels
        (2) Update MNEMONIC field of temp_instruction
        */
        strcpy(mnemonic, symbol);
        strcpy(temp_instruction->mnemonic, symbol);
        printf("=> mnemonic");
      }

      else if(strcmp(symbol, "syscall")==0){
        /*
        SYSCALL Definition
        (1) Update MNEMONIC field of temp_instruction
        */
        strcpy(temp_instruction->mnemonic, symbol);
        printf("=> syscall");
      }

      // PSEUDO INSTRUCTIONS
      else if(strcmp(mnemonic, "li") == 0){
        printf("=> operand");
        char first_input[100] = {};
        char second_input[100] = {};
        int j = 0, i = 0;
        j = 0;
        for(; symbol[i] != ','; i++, j++){
          first_input[j] = symbol[i];
        }

        j = 0;
        i++;
        for(; symbol[i] != '\n'; i++, j++){
        second_input[j] = symbol[i];
        }
        /*
        Decomposes to:
        lui $at,0x00001234
        ori $(target),$at,0x00005678
        */ 

        int upper = atoi(second_input), lower = atoi(second_input);
        upper = upper >> 16;
        lower = lower & 65535;

        strcpy(temp_instruction->mnemonic, "lui");
        temp_instruction->rs = REG_NUMBER("$at");
        temp_instruction->immediate = upper;
        InstructionList[INST_COUNTER++] = CREATE_INSTRUCTION(temp_instruction);

        free(temp_instruction);
        temp_instruction = RESET_TEMP();

        strcpy(temp_instruction->mnemonic, "ori");
        temp_instruction->rs = REG_NUMBER("$at");
        temp_instruction->rt = REG_NUMBER(first_input);
        temp_instruction->immediate = lower;
      }
      
      else if(strcmp(mnemonic, "la") == 0){
        printf("=> operand");
        char first_input[100] = {};
        char second_input[100] = {};
        char third_input[100] = {};
        int j = 0, i = 0;
        j = 0;
        i++;
        for(; symbol[i] != ','; i++, j++){
          first_input[j] = symbol[i];
        }

        j = 0;
        i++;
        for(; symbol[i] != '('; i++, j++){
          second_input[j] = symbol[i];
        }

        j = 0;
        i++;
        for(; symbol[i] != ')'; i++, j++){
          third_input[j] = symbol[i];
        }
        // NOT DONE
        /*
        Decomposes into:


        */
        temp_instruction->rt = REG_NUMBER(first_input);
        temp_instruction->immediate = atoi(second_input);
        temp_instruction->rs = REG_NUMBER(third_input);
      }

      else if(symbol[strlen(symbol)-1] == ')' && strcmp(mnemonic, "\0") == 0){
        char macro_type[20] = {};
        char first_input[100] = {};
        char second_input[100] = {};
        char third_input[100] = {};
        int i = 0, j = 0;
        for(; symbol[i] != '('; i++, j++){
          macro_type[j] = symbol[i];
        }

        strcpy(temp_instruction->mnemonic, macro_type);
        // Two Parameters
        if(strcmp(macro_type, "GCD") == 0){
          j = 0;
          i++;
          for(; symbol[i] != ','; i++, j++){
          first_input[j] = symbol[i];
          }

          j = 0;
          i++;
          for(; symbol[i] != ')'; i++, j++){
          second_input[j] = symbol[i];
          }

          temp_instruction->rs = REG_NUMBER(first_input);
          temp_instruction->rt = REG_NUMBER(second_input);
        }
        // One Parameters
        else if(strcmp(macro_type, "print_str") == 0){
          j = 0;
          i++;
          for(; symbol[i] != ')'; i++, j++){
          first_input[j] = symbol[i];
          }

          strcpy(temp_instruction->target, first_input);
          for(int m = 0; m < 4; m++){
            InstructionList[INST_COUNTER++] = CREATE_INSTRUCTION(temp_instruction);
            free(temp_instruction);  
            temp_instruction = RESET_TEMP();
            strcpy(temp_instruction->mnemonic, "macro");
          }
        }
        // Two Parameters
        else if(strcmp(macro_type, "read_str") == 0){
          j = 0;
          i++;
          for(; symbol[i] != ','; i++, j++){
          first_input[j] = symbol[i];
          }

          j = 0;
          i++;
          for(; symbol[i] != ')'; i++, j++){
          second_input[j] = symbol[i];
          }

          strcpy(temp_instruction->target, first_input);
          temp_instruction->immediate = atoi(second_input);
          for(int m = 0; m < 6; m++){
            InstructionList[INST_COUNTER++] = CREATE_INSTRUCTION(temp_instruction);
            free(temp_instruction);  
            temp_instruction = RESET_TEMP();
            strcpy(temp_instruction->mnemonic, "macro");
          }
        }
        // One Parameter
        else if(strcmp(macro_type, "print_integer") == 0){
          j = 0;
          i++;
          for(; symbol[i] != ')'; i++, j++){
          first_input[j] = symbol[i];
          }
          
          strcpy(temp_instruction->target, first_input);
        
          for(int m = 0; m < 4; m++){
            InstructionList[INST_COUNTER++] = CREATE_INSTRUCTION(temp_instruction);
            free(temp_instruction);  
            temp_instruction = RESET_TEMP();
            strcpy(temp_instruction->mnemonic, "macro");
          }
        }
        // One Parameter
        else if(strcmp(macro_type, "read_integer") == 0){
          j = 0;
          i++;
          for(; symbol[i] != ')'; i++, j++){
          first_input[j] = symbol[i];
          }

          temp_instruction->immediate = atoi(first_input);
          //NOT DONE
        }
        // No Parameter
        else if(strcmp(macro_type, "exit") == 0){
          
        }
        // ERROR CHECK
        else{
          printf("ERROR IN MACRO READ");
        }
      }
      
      else {
        /*
        OPERAND/S of the Instruction
        (1) Check if operand is R/I/J-Type
        (2) Scan the string to obtain individual operands
        -- REGISTER: use REG_NUMBER() to get corresponding number
        -- IMMEDIATE: use atoi() to convert string to decimal
        -- LABEL: add to symbol table
        -- Refer to GREEN SHEET for order of appearance of operands
        */
        if(IS_RTYPE(mnemonic)){
          // jr
          if(strcmp(mnemonic,"jr")==0){ // store to rs
            temp_instruction->rs = REG_NUMBER(symbol);
          }
          
          // move
          else if(strcmp(mnemonic,"move")==0){ // store to rd->rs($0)->rt
            char operand[5] = {};
            int i = 0, j = 0;
            
            strcpy(temp_instruction->mnemonic,"addu");
            strcpy(mnemonic,"addu");
            for(; symbol[i]!=','; i++, j++){
              operand[j] = symbol[i];
            }
            temp_instruction->rd = REG_NUMBER(operand);
            temp_instruction->rs = 0;
            temp_instruction->rt = REG_NUMBER(symbol+(i+1));
          }

          // add, sub, and, or, slt
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

              for(i=i+1, j=0; symbol[i]!='\0' && symbol[i]!='('; i++, j++)
                operand[j] = symbol[i];
              temp_instruction->immediate = atoi(operand);

              // lw, sw
              if(mnemonic[1] == 'w'){ // store to rs
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

            if(SYMBOL_EXISTS(symbol, head) == -1) APPEND_SYMBOL(symbol, 0, &head);
            strcpy(temp_instruction->target, symbol+(i+1));
            printf("%s", temp_instruction->target);
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
        }

        else{ // j-type
          if(SYMBOL_EXISTS(symbol, head) == -1)
            APPEND_SYMBOL(symbol, 0, &head);
          strcpy(temp_instruction->target, symbol);
          printf("%s",temp_instruction->target);
        }

        printf("=> %s",IS_JTYPE(mnemonic) ? "label" : "operand");
      }
    }

    char x = c; // temporarily store value of delimiter for later usage
    int IS_DATA = strcmp(symbol,".data")==0;
    int to_break = fscanf(fp, "%s%c", &symbol, &c);   // continue scan
                                                      // to_break == EOF if end of file was reached
    
    if(x == '\n' || to_break==EOF){
      /*
      END OF LINE has already been reached
      (1) Create an INSTRUCTION from collected data
      -- Assign instruction to corresponding line number
      -- NULL/Default values will be used for non-instructions
      (2) Reset values
      -- Set 'mnemonic' to NULL ("\0")
      -- Free temp_instruction and re-initialize
      (3) Check if EOF has been reached
      -- YES  =>  BREAK
      -- NO   =>  PRINT new line
      */

      // (1)
      if(strcmp(temp_instruction->mnemonic,"\0")!=0)
        InstructionList[INST_COUNTER++] = CREATE_INSTRUCTION(temp_instruction);

      printf("\nINST_COUNTER: %d",INST_COUNTER);
      if(INST_COUNTER != 0) if(INST_COUNTER!=0) printf("%s",InstructionList[INST_COUNTER-1]->mnemonic);

      // (2)
      strcpy(mnemonic,"\0");
      free(temp_instruction);
      temp_instruction = RESET_TEMP();

      if(IN_DATA_SEGMENT == 1 && !IS_DATA){
    		ADD_TO_MEMORY(MemoryFile, head);
      }

      // (3)
      if(to_break==EOF)
        break;
      printf("\n[Line %d]", ++LINE_NUMBER); 

    }
  }
  PRINT_INSTRUCTIONS(InstructionList, INST_COUNTER);
  PRINT_SYMBOL_TABLE(head, output);
  rewind(output);
  PRINT_DATA_SEGMENT(head);
  PRINT_MEMORY(MemoryFile, BYTE_COUNTER);

  // SECOND PASS
  int machine_code;
  for (int line = 0; line < INST_COUNTER; line++){
    if (strcmp(InstructionList[line]->mnemonic, "macro") == 0) continue;
    char *machine_code_string;
		if (IS_RTYPE(InstructionList[line]->mnemonic)){
      /*
      R-TYPE INSTRUCTION
      |000000|XXXXX|XXXXX|XXXXX|00000|XXXXXX|
      |opcode|rs   |rt   |rd   |shamt|funct |
      |31:26 |25:21|20:16|15:11|10:6 |5:0   |
      */

    	if (strcmp(InstructionList[line]->mnemonic, "add") == 0) machine_code = 32;
			else if (strcmp(InstructionList[line]->mnemonic, "sub") == 0) machine_code = 34;
			else if (strcmp(InstructionList[line]->mnemonic, "and") == 0) machine_code = 36;
			else if (strcmp(InstructionList[line]->mnemonic, "or") == 0) machine_code = 37;
			else if (strcmp(InstructionList[line]->mnemonic, "slt") == 0) machine_code = 42;

			machine_code = machine_code | (InstructionList[line]->rd << 11);
			machine_code = machine_code | (InstructionList[line]->rt << 16);
			machine_code = machine_code | (InstructionList[line]->rs << 21);
		}

    else if(IS_ITYPE(InstructionList[line]->mnemonic)){
      /*
      I-TYPE INSTRUCTION
      |000000|XXXXX|XXXXX|XXXXXXXXXXXXXXXX|
      |funct |rs   |rt   |immediate       |
      |31:26 |25:21|20:16|15:0            |
      */
    	if (strcmp(InstructionList[line]->mnemonic, "addi") == 0) machine_code = InstructionList[line]->immediate + (8 << 26);
    	else if (strcmp(InstructionList[line]->mnemonic, "addiu") == 0) machine_code = InstructionList[line]->immediate + (9 << 26);
    	else if (strcmp(InstructionList[line]->mnemonic, "lui") == 0) machine_code = InstructionList[line]->immediate + (15 << 26);
    	else if (strcmp(InstructionList[line]->mnemonic, "lw") == 0){
        if (strcmp(InstructionList[line]->target,"\0") == 0) machine_code = InstructionList[line]->immediate + (35 << 26);
        else machine_code = SYMBOL_EXISTS(InstructionList[line]->target,head) - 0x10000000 + (35 << 26);
      } 
    	else if (strcmp(InstructionList[line]->mnemonic, "ori") == 0){
        if (strcmp(InstructionList[line]->target,"\0") == 0) machine_code = InstructionList[line]->immediate + (13 << 26);
        else machine_code = SYMBOL_EXISTS(InstructionList[line]->target,head) - 0x10000000 + (13 << 26);
      }
    	else if (strcmp(InstructionList[line]->mnemonic, "sw") == 0) machine_code = InstructionList[line]->immediate + (45 << 26);
    	else if (strcmp(InstructionList[line]->mnemonic, "beq") == 0) machine_code = 0; // Not Done
    	else if (strcmp(InstructionList[line]->mnemonic, "bne") == 0) machine_code = 0; // Not Done

      machine_code = machine_code | (InstructionList[line]->rt << 16);
      machine_code = machine_code | (InstructionList[line]->rs << 21);
    }

    else if(IS_JTYPE(InstructionList[line]->mnemonic)){
      if (strcmp(InstructionList[line]->mnemonic, "j") == 0); // Not Done
      if (strcmp(InstructionList[line]->mnemonic, "jal") == 0); // Not Done
    }

    else if(IS_MACRO(InstructionList[line]->mnemonic)){
      if (strcmp(InstructionList[line]->mnemonic, "GCD") == 0) machine_code = 201342976;
      else if (strcmp(InstructionList[line]->mnemonic, "print_str") == 0){
        /*
        Decomposes into: 
        la $a0,%label               (label address: 0x12345678)
          | lui $at,0x00001234       (001111 00000 00001 XXXXXXXXXXXXXXXX)
          | ori $a0,$at,0x00005678   (001101 00001 00100 XXXXXXXXXXXXXXXX)
        li $v0,4
          | lui $at,0x00000000       (001111 00000 00001 0000000000000000)
          | ori $v0,$at,0x00000004   (001101 00001 00010 0000000000000100)
        syscall
        */
        //la
        int address = SYMBOL_EXISTS(InstructionList[line]->target, head);
        int upper = address; int lower = address;
        upper = upper >> 16;
        lower = lower & 65535;
        
        machine_code = (15361 << 16) + upper;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = (13348 << 16) + lower;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);
        //li $v0,4
        machine_code = 1006698496;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = 874643460;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);
        //syscall
        machine_code = 0;
      }
      else if(strcmp(InstructionList[line]->mnemonic, "read_str") == 0){
        /*
        Decomposes into:
        la $a0, %label               (label address: 0x12345678)
          | lui $at,0x00001234       (001111 00000 00001 XXXXXXXXXXXXXXXX)
          | ori $a0,$at,0x00005678   (001101 00001 00100 XXXXXXXXXXXXXXXX)
        li $a1, 0x12345678           
          | lui $at,0x00001234       (001111 00000 00001 XXXXXXXXXXXXXXXX)
          | ori $a1,$at,0x00005678   (001101 00001 00101 XXXXXXXXXXXXXXXX)
        li $v0, 8
          | lui $at,0x00000000       (001111 00000 00001 0000000000000000)
          | ori $v0,$at,0x00000008   (001101 00001 00010 0000000000001000)
        syscall
        */
        // la $a0,%label
        int address = GET_SYMBOL_ADDRESS(InstructionList[line]->target, output);
        int upper = address; int lower = address;
        upper = upper >> 16;
        lower = lower & 65535;

        machine_code = (15361 << 16) + upper;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = (13348 << 16) + lower;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);
        //li $a1,0x12345678
        upper = InstructionList[line]->immediate >> 16;
        lower = InstructionList[line]->immediate & 65535;

        machine_code = (15361 << 16) + upper;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = (13349 << 16) + lower;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);
        //li $v0,8
        machine_code = 1006698496;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = 874643464;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);
        //syscall
        machine_code = 0;
      }
      else if(strcmp(InstructionList[line]->mnemonic, "print_integer") == 0){
        /*
        Decomposes into:
        la $a0, %label               (label address: 0x12345678)
          | lui $at,0x00001234       (001111 00000 00001 XXXXXXXXXXXXXXXX)
          | ori $a0,$at,0x00005678   (001101 00001 00100 XXXXXXXXXXXXXXXX)
        li $v0, 1
          | lui $at,0x00000000       (001111 00000 00001 0000000000000000)
          | ori $v0,$at,0x00000001   (001101 00001 00010 0000000000000001)
        syscall
        */
        //la $a0,%label
        int address = GET_SYMBOL_ADDRESS(InstructionList[line]->target, output);
        int upper = address; int lower = address;
        upper = upper >> 16;
        lower = lower & 65535;

        machine_code = (15361 << 16) + upper;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = (13348 << 16) + lower;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);
        //li $v0,1
        machine_code = 1006698496;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        machine_code = 874643457;
        machine_code_string = GET_BINARY(machine_code);
        fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
        fprintf(machinecode, "%0*s\n", 32, machine_code_string);

        //syscall
        machine_code = 0;
      }
      else if(strcmp(InstructionList[line]->mnemonic, "read_integer") == 0){
        /*
        Decomposes into:

        */
      }
    }
		machine_code_string = GET_BINARY(machine_code);
    fprintf(machinecode, "%s: ", InstructionList[line]->mnemonic);
    fprintf(machinecode, "%0*s\n", 32, machine_code_string);
  }
  return 0;
}

void ADD_TO_MEMORY(char MemoryFile[], Symbol *SymbolTable){
  Symbol *data_label = SymbolTable;
  while(data_label->next) data_label = data_label->next;
  if(strcmp(data_label->str_value,"\0")!=0) // string
    strcpy(MemoryFile + (data_label->address - BASE_DATA), data_label->str_value);
  else{ // integer
    for(int i=0; i<4; i++)
      MemoryFile[data_label->address - BASE_DATA + i] = data_label->int_value >> (32-(((i+1))*8));
  }
}

int LOAD_INT(char MemoryFile[], int address){
  int value = 0;
  for(int start = address - BASE_DATA, i=0; i<4; start++, i++)
    value += ((int) MemoryFile[start] << 8*(3-i));
  return value;
}

void PRINT_MEMORY(char MemoryFile[], int BYTE_COUNTER){
  int total = (((int)(BYTE_COUNTER/4))+1)*4;
  printf("\n\nMemory Array\n");
  for(int i=0; i<total/4; i++){
      for(int j=0; j<4; j++)
          printf("| %02X %c", MemoryFile[4*i+j], j==3 ? '|' : '\0');
      printf("\n");
  }

  printf("\nASCII\n");
  for(int i=0; i<total/4; i++){
      for(int j=0; j<4; j++)
          printf("| %c %c", MemoryFile[4*i+j] == '\0' ? ' ' : MemoryFile[4*i+j], j==3 ? '|' : '\0');
      printf("\n");
  }
}

void PRINT_INSTRUCTIONS(Instruction *InstructionList[], int N){
  printf("\n\nScanned the following instructions...\n");
  for(int i = 0; i < N; i++)
  {
    Instruction *inst = InstructionList[i];
    if(strcmp(inst->mnemonic,"\0")){
      printf("[%d] %s ", i+1,(InstructionList[i])->mnemonic);
      if(inst->rd != -1) printf("[rd: %d] ", inst->rd);
      if(inst->rt != -1) printf("[rt: %d] ", inst->rt);
      if(inst->rs != -1) printf("[rs: %d] ", inst->rs);
      if((IS_ITYPE(inst->mnemonic) && inst->mnemonic[0]!='b') || IS_PSEUDO(inst->mnemonic)) printf("[imm: %d] ", inst->immediate);
      if(strcmp(inst->target,"\0")!=0) printf("[target: %s] ", inst->target);
      printf("\n");
    }
  }
}

void PRINT_SYMBOL_TABLE(Symbol *head, FILE *output){
  printf("\nSymbol Table:\n");
  Symbol *temp = head;
  while(temp){
    printf("[%-5s 0x%08X]", temp->name, temp->address);
    fprintf(output, "%s\t0x%08X", temp->name, temp->address); // write output to output.txt
    if(temp->next) printf("\n");
    if(temp->next) fprintf(output,"\n"); // write output to output.txt
    temp = temp->next;
  }
}

void PRINT_DATA_SEGMENT(Symbol *head){
  printf("\n\nData Segment Labels:\n");
  Symbol *temp = head;
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
    if(temp->next && printed) printf("\n");
    temp = temp->next;
  }
}

Instruction* CREATE_INSTRUCTION(Instruction *temp){
  Instruction *A = (Instruction *)malloc(sizeof(Instruction));
  strcpy(A->mnemonic, temp->mnemonic);
  strcpy(A->target, temp->target);
  A->rs = temp->rs;
  A->rt = temp->rt;
  A->rd = temp->rd;
  A->immediate = temp->immediate;
  return A;
}

Instruction* RESET_TEMP(){
  Instruction *temp = (Instruction *)malloc(sizeof(Instruction));
  strcpy(temp->mnemonic,"\0");
  strcpy(temp->target,"\0");
  temp->rs = -1;
  temp->rt = -1;
  temp->rd = -1;
  temp->immediate = -1;
  return temp;
}

int NEEDS_TARGET(char mnem[]){
  // check if mnemonic of instruction associated with operand needs target
  char lst[6][5] = {"beq", "bne", "lw", "sw", "j", "jal"};
  for(int i=0; i<6; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
  return 0;
}

int IS_PSEUDO(char mnem[]){
// check if mnemonic of instruction is pseudoinstruction
  char lst[7][5] = {"li", "la"};
  for(int i=0; i<2; i++)
    if(strcmp(lst[i], mnem)==0) return 1;
  return 0;
}

int IS_MACRO(char mnem[]){
  // check if menmonic of instruction is a macro
  char lst[6][15] = {"GCD","print_str","read_str","print_integer","read_integer","exit"};
  for(int i = 0; i < 6; i++)
    if(strcmp(lst[i], mnem) == 0) return 1;
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
  char lst[8][6] = {"addi","addiu","lui","lw","ori","sw","beq","bne"};
  for(int i=0; i<8; i++)
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

int HEX_TO_DECIMAL(char* hex){
  int integer; int exp = 0; int decimal = 0;
  sscanf(hex, "0x%08d", &integer);
  for(int count = 0; count < 8; count++){
    decimal = decimal + (integer % 10)*(pow(16,exp++)); 
    integer = integer/10;
  }
  printf("%d", decimal);
  return decimal;
}

void APPEND_SYMBOL(char symbol_name[], int address_val, Symbol **head){
  // create new symbol for label
  Symbol *new_symbol = (Symbol *)malloc(sizeof(Symbol));
  strcpy(new_symbol->name, symbol_name);
  strcpy(new_symbol->str_value, "\0");
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

Symbol *GET_TAIL(Symbol *head){
  Symbol *temp = head;
  while(temp->next)
    temp = temp->next;
  return temp;
}

void UPDATE_STR(char str_value[], Symbol *head){
  /*
  Update str_value field of SYMBOL in Data Segment
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
  Update str_value field of SYMBOL in Data Segment
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
    if(strcmp(symbol_name,temp->name)==0) return temp->address;
    temp = temp->next;
  }
  return -1;
}

char *GET_BINARY(int number){
    static char BINARY_STR[33] = {};
    for(int i=31; i>=0; number/=2, i--)
        BINARY_STR[i] = number%2 == 0 ? '0' : '1';
    return BINARY_STR;
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
