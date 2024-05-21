lines_of_code = """// LOAD WORD FROM STACK
StackNode* temp_pointer = StackPointer;
for (int count = 0; count < (InstructionList[line]->immediate / 4); count++)
    temp_pointer = temp_pointer->prev;
RegisterFile[InstructionList[line]->rt] = temp_pointer->data;

// STORE WORD TO STACK

StackNode* temp_pointer = StackPointer;
for (int count = 0; count < (InstructionList[line]->immediate / 4); count++)
    temp_pointer = temp_pointer->prev;
temp_pointer->data = RegisterFile[InstructionList[line]->rt];"""
for x in lines_of_code.split('\n'):
    temp = x.replace("_", "\\_")
    temp = temp.replace("$","\\$")
    temp = temp.replace("{","\\{")
    temp = temp.replace("}","\\}")
    print("\\texttt{" + f"{temp}" + "}\\\\")