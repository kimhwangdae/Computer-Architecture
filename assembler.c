#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

/*
 * For debug option. If you want to debug, set 1.
 * If not, set 0.
 */
#define DEBUG 0
#define MAX_SYMBOL_TABLE_SIZE   1024
#define MEM_TEXT_START          0x00400000
#define MEM_DATA_START          0x10000000
#define BYTES_PER_WORD          4
#define INST_LIST_LEN           20

/******************************************************
 * Structure Declaration
 *******************************************************/

typedef struct inst_struct {
    char *name;
    char *op;
    char type;
    char *funct;
} inst_t;

typedef struct symbol_struct {
    char name[32];
    uint32_t address;
} symbol_t;

enum section { 
    DATA = 0,
    TEXT,
    MAX_SIZE
};

/******************************************************
 * Global Variable Declaration
 *******************************************************/

inst_t inst_list[INST_LIST_LEN] = {       //  idx
    {"addiu",   "001001", 'I', ""},       //    0
    {"addu",    "000000", 'R', "100001"}, //    1
    {"and",     "000000", 'R', "100100"}, //    2
    {"andi",    "001100", 'I', ""},       //    3
    {"beq",     "000100", 'I', ""},       //    4
    {"bne",     "000101", 'I', ""},       //    5
    {"j",       "000010", 'J', ""},       //    6
    {"jal",     "000011", 'J', ""},       //    7
    {"jr",      "000000", 'R', "001000"}, //    8
    {"lui",     "001111", 'I', ""},       //    9
    {"lw",      "100011", 'I', ""},       //   10
    {"nor",     "000000", 'R', "100111"}, //   11
    {"or",      "000000", 'R', "100101"}, //   12
    {"ori",     "001101", 'I', ""},       //   13
    {"sltiu",   "001011", 'I', ""},       //   14
    {"sltu",    "000000", 'R', "101011"}, //   15
    {"sll",     "000000", 'R', "000000"}, //   16
    {"srl",     "000000", 'R', "000010"}, //   17
    {"sw",      "101011", 'I', ""},       //   18
    {"subu",    "000000", 'R', "100011"}  //   19
};

symbol_t SYMBOL_TABLE[MAX_SYMBOL_TABLE_SIZE]; // Global Symbol Table

uint32_t symbol_table_cur_index = 0; // For indexing of symbol table
// uint32_t is not signedint (4byte)
/* Temporary file stream pointers */
FILE *data_seg;
FILE *text_seg;

/* Size of each section */
uint32_t data_section_size = 0;
uint32_t text_section_size = 0;

/******************************************************
 * Function Declaration
 *******************************************************/

/* Change file extension from ".s" to ".o" */
char* change_file_ext(char *str) {
    char *dot = strrchr(str, '.');

    if (!dot || dot == str || (strcmp(dot, ".s") != 0))
        return NULL;

    str[strlen(str) - 1] = 'o';
    return "";
}

/* Add symbol to global symbol table */
void symbol_table_add_entry(symbol_t symbol)
{
    SYMBOL_TABLE[symbol_table_cur_index++] = symbol;
#if DEBUG
    printf("%s: 0x%08x\n", symbol.name, symbol.address);
#endif
}

/* Convert integer number to binary string */
char* num_to_bits(unsigned int num, int len) 
{
    char* bits = (char *) malloc(len+1);
    int idx = len-1, i;
    while (num > 0 && idx >= 0) {
        if (num % 2 == 1) {
            bits[idx--] = '1';
        } else {
            bits[idx--] = '0';
        }
        num /= 2;
    }
    for (i = idx; i >= 0; i--){
        bits[i] = '0';
    }
    bits[len] = '\0';
    return bits;
}

/* Record .text section to output file */
void record_text_section(FILE *output) 
{
    uint32_t cur_addr = MEM_TEXT_START;
    char line[1024];

    /* Point to text_seg stream */
    rewind(text_seg);

    /* Print .text section */
    while (fgets(line, 1024, text_seg) != NULL) {
        char inst[0x1000] = {0};
        char op[32] = {0};
        char label[32] = {0};
        char type = '0';
        int i, idx = 0;
        char rs[1024]={0};
        char rt[1024]={0};
        char rd[1024]={0};
        char imm[1024]={0};
        char shamt[1024]={0};
        char funct[1024]={0};
        char addr[1024]={0};
        char *temp;
        char *temp_ptr;
        strcpy(inst,line);
        temp=strtok(inst,"/");
        if(temp==NULL)
            break;

        for (i = 0; i < INST_LIST_LEN; i++)
        {
            if(strcmp(temp,inst_list[i].name)==0){
                strcpy(op,inst_list[i].op);
                type=inst_list[i].type;
                break;
            }
        }
        
#if DEBUG
        printf("0x%08x: ", cur_addr);
#endif
        /* Find the instruction type that matches the line */
        /* blank */

        switch (type) {
            case 'R':
                /* blank */
                temp=strtok(NULL,"/");
                temp_ptr=strchr(temp,'$');
                if(temp_ptr[strlen(temp_ptr)-1]==','){
                    temp_ptr[strlen(temp_ptr)-1]=0;
                }
                strcpy(rd,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));
               // printf("RD:%s\n",rd);
                if(i==8){
                    strcpy(rt,"00000");
                    strcpy(rs,"00000");
                    strcpy(shamt,"00000");
                    strcpy(funct,inst_list[i].funct);
                    fprintf(output,"%s%s%s%s%s%s",op,rd,rt,rs,shamt,funct);
                    break;
                }
                temp=strtok(NULL,"/");
                temp_ptr=strchr(temp,'$');
                if(temp_ptr[strlen(temp_ptr)-1]==','){
                    temp_ptr[strlen(temp_ptr)-1]=0;
                }
                strcpy(rs,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));
               // printf("RS:%s\n",rs);
                temp=strtok(NULL,"/");
                temp_ptr=strchr(temp,'$');
                if(temp_ptr==NULL){
                    strcpy(shamt,num_to_bits((int)strtol(temp,NULL,0),5));
                    strcpy(rt,rs);
                    strcpy(rs,"00000");
                      
                }
                else{
                    strcpy(rt,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));    
                    strcpy(shamt,"00000");
                }
               // printf("RT:%s\n",rt);
               // printf("Shamt:%s\n",shamt);
                strcpy(funct,inst_list[i].funct);

                fprintf(output,"%s%s%s%s%s%s",op,rs,rt,rd,shamt,funct);

#if DEBUG
                printf("op:%s rs:$%d rt:$%d rd:$%d shamt:%d funct:%s\n",
                        op, rs, rt, rd, shamt, inst_list[idx].funct);
#endif
                break;

            case 'I':
                /* blank */
                temp=strtok(NULL,"/");
                temp_ptr=strchr(temp,'$');
                if(temp_ptr[strlen(temp_ptr)-1]==','){
                    temp_ptr[strlen(temp_ptr)-1]=0;
                }
                strcpy(rs,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));
               // printf("I-code:%s\n",rs);
                if(i==9){
                    temp=strtok(NULL,"/");
                    if(temp[strlen(temp)-1]=='\n'){
                        temp[strlen(temp)-1]=0;
                    }
                    strcpy(imm,num_to_bits((int)strtol(temp,NULL,0),16)); 
                    strcpy(rt,"00000");
                    fprintf(output,"%s%s%s%s",op,rt,rs,imm);
                }
                else if(i==4||i==5){
                    temp=strtok(NULL,"/");
                    temp_ptr=strchr(temp,'$');
                    if(temp_ptr[strlen(temp_ptr)-1]==','){
                    temp_ptr[strlen(temp_ptr)-1]=0;
                    }
                    strcpy(rt,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));
                    temp=strtok(NULL,"/");
                    if(temp[strlen(temp)-1]=='\n'){
                        temp[strlen(temp)-1]=0;
                    }
                    for (int j = 0; j <symbol_table_cur_index ; j++)
                     {
                        if(strcmp(temp,SYMBOL_TABLE[j].name)==0){

                            strcpy(imm,num_to_bits(SYMBOL_TABLE[j].address-(cur_addr/4)-1,16));
                            break;
                        }
                    }
               //     printf("beq or ben: %s \n",rt);
              //      printf("beq or ben is imm :%s\n ",imm);
                    fprintf(output,"%s%s%s%s",op,rs,rt,imm);
                }
                else if(i==10||i==18){
                    temp=strtok(NULL,"/");
                    strcpy(imm,num_to_bits((int)strtol(temp,NULL,0),16));
             //       printf("%s",temp);
                    temp_ptr=strchr(temp,'$');
                    if(temp_ptr[strlen(temp_ptr)-1]=='\n'){
                       temp_ptr[strlen(temp_ptr)-1]=0;
                    }
           //         printf("%s\n",temp_ptr);
                    strcpy(rt,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));
         //           printf("rt:%s\n",rs);
                    fprintf(output,"%s%s%s%s",op,rt,rs,imm);
                }
                else{
                    temp=strtok(NULL,"/");
                    temp_ptr=strchr(temp,'$');
                    if(temp_ptr[strlen(temp_ptr)-1]==','){
                    temp_ptr[strlen(temp_ptr)-1]=0;
                    }
                    strcpy(rt,num_to_bits((int)strtol(temp_ptr+1,NULL,0),5));
                    temp=strtok(NULL,"/");
                    if(temp[strlen(temp)-1]=='\n'){
                        temp[strlen(temp)-1]=0;
                    }
                    strcpy(imm,num_to_bits((int)strtol(temp,NULL,0),16));
                    
                    fprintf(output,"%s%s%s%s",op,rt,rs,imm);
                }
#if DEBUG
                printf("op:%s rs:$%d rt:$%d imm:0x%x\n",
                        op, rs, rt, imm);
#endif
                break;

            case 'J':
                /* blank */
                temp=strtok(NULL,"/");
                temp[strlen(temp)-1]=0;
                for (int j = 0; j <symbol_table_cur_index ; j++)
                {
                    if(strcmp(temp,SYMBOL_TABLE[j].name)==0){

                        strcpy(addr,num_to_bits(SYMBOL_TABLE[j].address,26));
                        break;
                    }
                    
                }
                fprintf(output,"%s%s",op,addr);
                


#if DEBUG
                printf("op:%s addr:%i\n", op, addr);
#endif
                break;

            default:
                break;
        }
        fprintf(output, "\n");

        cur_addr += BYTES_PER_WORD;
    }
}

/* Record .data section to output file */
void record_data_section(FILE *output)
{
    uint32_t cur_addr = MEM_DATA_START;
    char line[1024];

    /* Point to data segment stream */
    rewind(data_seg);

    /* Print .data section */
    while (fgets(line, 1024, data_seg) != NULL) {
        /* blank */
        char temp_data[1024]={0};
        char *temp;
        char input_data[1024]={0};
        strcpy(temp_data,line);
        temp=temp_data;
        strcpy(input_data,num_to_bits((int)strtol(temp,NULL,0),32));
        fprintf(output,"%s\n",input_data);
#if DEBUG
        printf("0x%08x: ", cur_addr);
        printf("%s", line);
#endif
        cur_addr += BYTES_PER_WORD;
    }
}

/* Fill the blanks */
void make_binary_file(FILE *output)
{
#if DEBUG1
    char line[1024] = {0};
    rewind(text_seg);
    /* Print line of text segment */
    while (fgets(line, 1024, text_seg) != NULL) {
        printf("%s",line);
    }
    printf("text section size: %d, data section size: %d\n",
            text_section_size, data_section_size);
#endif

    /* Print text section size and data section size */
    /* blank */

        char text_temp[1024];
        fseek(text_seg,0,SEEK_SET);
        for(int i=0;fgets(text_temp,sizeof(text_temp),text_seg)!=NULL;)
        {
       //     printf("text_temp:%s",text_temp);
            
        }
      //  printf("text_section_size:%d\n",text_section_size);
    
    fprintf(output,"%s\n",num_to_bits(text_section_size,32));
    fprintf(output,"%s\n",num_to_bits(data_section_size,32));
    
    /* Print .text section */
    record_text_section(output);

    /* Print .data section */
    record_data_section(output);
}

/* Fill the blanks */



void make_symbol_table(FILE *input,char *filename)
{
    char line[1024] = {0};
    uint32_t address = 0;
    enum section cur_section = MAX_SIZE;
    int flag=0;
    char *filename_temp=filename;
    filename_temp=strtok(filename_temp,"/");
    filename_temp=strtok(NULL,".");
                        
    /* Read each section and put the stream */
    while (fgets(line, 1024, input) != NULL) {
        char *temp;
        char _line[1024] = {0};
        strcpy(_line, line);
        temp = strtok(_line, "\t\n");
        /* Check section type */
        if (!strcmp(temp, ".data")) {
            /* blank */            
            cur_section=DATA;
            address=MEM_DATA_START;
            data_seg = tmpfile();
            continue;

        }
        else if (!strcmp(temp, ".text")) {
            /* blank */;
            data_section_size=address-MEM_DATA_START;
      //      printf("data_section_size:%d\n",data_section_size);
            address=MEM_TEXT_START; 
            cur_section=TEXT;
            text_seg = tmpfile();
            continue;
        }

        /* Put the line into each segment stream */
        if (cur_section == DATA) {
            /* blank */
            while(temp!=NULL){
                if(temp[strlen(temp)-1]==':'){
                    temp[strlen(temp)-1]=0;
                    strcpy(SYMBOL_TABLE[symbol_table_cur_index].name,temp);
                    SYMBOL_TABLE[symbol_table_cur_index].address=address;
                   // printf("%s \n",SYMBOL_TABLE[symbol_table_cur_index].name);
                   // printf("%u \n",SYMBOL_TABLE[symbol_table_cur_index].address);
                    symbol_table_cur_index++;
                }
                else{
                    if(temp[0]=='.'){
                   //     printf("pre temp:%s\n",temp);
                    //    if(filename_temp[strlen(filename_temp)-1]=='7'){
                          
                     //   }

                         temp=strtok(NULL,"   ");
                     //   printf("post temp:%s\n",temp);
                        continue;        
                    } 
                    fprintf(data_seg,"%s",temp);
                }
                if(filename_temp[strlen(filename_temp)-1]=='7'){
                 temp=strtok(NULL," ");             
                }
                temp=strtok(NULL,"	");
            }
        }

        
        else if (cur_section == TEXT) {
            /* blank */
            flag=0;
            while(temp!=NULL){
                if(temp[strlen(temp)-1]==':'){
                    temp[strlen(temp)-1]=0;
                    strcpy(SYMBOL_TABLE[symbol_table_cur_index].name,temp);
                    SYMBOL_TABLE[symbol_table_cur_index].address=address/4;
                   // printf("%s \n",SYMBOL_TABLE[symbol_table_cur_index].name);
                   // printf("%u \n",SYMBOL_TABLE[symbol_table_cur_index].address);
                    symbol_table_cur_index++;
                    flag=1;                
                }
                else{
                    if(strcmp(temp,"la")==0){
                        
                       // printf("this is la\n");
                        char temp_text_t[1024];
                        char temp_text_d[1024];
                        uint32_t ori_address;
                        int is_only_lui=1;
                        temp=strtok(NULL," ");
                        strcpy(temp_text_t,temp);
                        temp=strtok(NULL,"\n");
                        strcpy(temp_text_d,temp);
                     //   printf("%s\n",temp_text_d);
                        for (int i = 0; i < symbol_table_cur_index; i++)
                        {
                            if(strcmp(temp_text_d,SYMBOL_TABLE[i].name)==0){
                                ori_address=SYMBOL_TABLE[i].address;
                                char check[33];
                                strcpy(check,num_to_bits(SYMBOL_TABLE[i].address,32));
                         //       printf("%s\n",check);
                                for (int j = 0; j < 32; ++j)
                                {
                                    if(j<16){
                                        temp_text_d[j]=check[j];
                                    }
                                    else{
                                        if(check[j]!='0'){
                                            is_only_lui=0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                        }

                       // printf("%s\n",temp_text_d);
                        temp_text_d[16]=NULL;
                        uint32_t dec=strtol(temp_text_d,NULL,2);
                      //  printf("this is dec: %u \n",dec);
                        fputs("lui/",text_seg);
                        fprintf(text_seg,"%s/",temp_text_t);
                        fprintf(text_seg,"0x%x\n",dec);
                        if(is_only_lui==0){
                            fputs("/ori/",text_seg);
                            fprintf(text_seg,"%s/",temp_text_t);
                            fprintf(text_seg,"%s/",temp_text_t);
                         //   printf("%u\n",ori_address);
                            fprintf(text_seg,"0x%x\t\n",ori_address-MEM_DATA_START);
                            address+=BYTES_PER_WORD;
                        }


                    }
                    else{
                        fprintf(text_seg,"%s/",temp);
                            
                    }
                }
                temp=strtok(NULL," ");
            }
        }

        if(flag==1){
            continue;
        }
        address += BYTES_PER_WORD;
    }
    text_section_size=address-MEM_TEXT_START;
}

/******************************************************
 * Function: main
 *
 * Parameters:
 *  int
 *      argc: the number of argument
 *  char*
 *      argv[]: array of a sting argument
 *
 * Return:
 *  return success exit value
 *
 * Info:
 *  The typical main function in C language.
 *  It reads system arguments from terminal (or commands)
 *  and parse an assembly file(*.s).
 *  Then, it converts a certain instruction into
 *  object code which is basically binary code.
 *
 *******************************************************/

int main(int argc, char* argv[])
{
    FILE *input, *output;
    char *filename;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <*.s>\n", argv[0]);
        fprintf(stderr, "Example: %s sample_input/example?.s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Read the input file */
    input = fopen(argv[1], "r");
    if (input == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /* Create the output file (*.o) */
    filename = strdup(argv[1]); // strdup() is not a standard C library but fairy used a lot.
    if(change_file_ext(filename) == NULL) {
        fprintf(stderr, "'%s' file is not an assembly file.\n", filename);
        exit(EXIT_FAILURE);
    }
    printf("%s\n",filename);
    output = fopen(filename, "w");
    if (output == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /******************************************************
     *  Let's complete the below functions!
     *
     *  make_symbol_table(FILE *input)
     *  make_binary_file(FILE *output)
     *  ├── record_text_section(FILE *output)
     *  └── record_data_section(FILE *output)
     *
     *******************************************************/
    make_symbol_table(input,filename);
    make_binary_file(output);

    fclose(input);
    fclose(output);

    return 0;
}
