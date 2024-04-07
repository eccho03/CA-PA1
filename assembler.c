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
#define INST_LIST_LEN           22

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
    uint32_t address; //uint32_t means unsigned int
} symbol_t;

enum section {
    DATA = 0,
    TEXT,
    MAX_SIZE //DATA = 0, TEXT = 1, MAX_SIZE = 2
};

/******************************************************
 * Global Variable Declaration
 *******************************************************/

inst_t inst_list[INST_LIST_LEN] = {       //  idx
    {"add",     "000000", 'R', "100000"}, //    0
    {"sub",     "000000", 'R', "100010"}, //    1
    {"addiu",   "001001", 'I', ""},       //    2
    {"addu",    "000000", 'R', "100001"}, //    3
    {"and",     "000000", 'R', "100100"}, //    4
    {"andi",    "001100", 'I', ""},       //    5
    {"beq",     "000100", 'I', ""},       //    6
    {"bne",     "000101", 'I', ""},       //    7
    {"j",       "000010", 'J', ""},       //    8
    {"jal",     "000011", 'J', ""},       //    9
    {"jr",      "000000", 'R', "001000"}, //   10
    {"lui",     "001111", 'I', ""},       //   11
    {"lw",      "100011", 'I', ""},       //   12
    {"nor",     "000000", 'R', "100111"}, //   13
    {"or",      "000000", 'R', "100101"}, //   14
    {"ori",     "001101", 'I', ""},       //   15
    {"sltiu",   "001011", 'I', ""},       //   16
    {"sltu",    "000000", 'R', "101011"}, //   17
    {"sll",     "000000", 'R', "000000"}, //   18
    {"srl",     "000000", 'R', "000010"}, //   19
    {"sw",      "101011", 'I', ""},       //   20
    {"subu",    "000000", 'R', "100011"}  //   21
};

symbol_t SYMBOL_TABLE[MAX_SYMBOL_TABLE_SIZE]; // Global Symbol Table

uint32_t symbol_table_cur_index = 0; // For indexing of symbol table

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
        char label[32] = {0}; //data1 이런거
        char type = '0';
        int i, idx = 0;
        int rs, rt, rd, imm, shamt;
        int addr;

        rs = rt = rd = imm = shamt = addr = 0;
#if DEBUG
        printf("0x%08x: ", cur_addr);
#endif
        /* Find the instruction type that matches the line */
        /* blank */
	    char *temp;
	    temp = strtok(line, " \t\n"); //and lui 이런거 떼어냄
	    for (idx = 0; idx < INST_LIST_LEN; idx++) {
	        if (!strcmp(temp, inst_list[idx].name)) {
		        break;
	        }
        }		
		type = inst_list[idx].type; //and lui 이게 무슨 타입인지 찾음
        //printf("%d ---\n", idx);
	        	    
	//-------------------------------------//
        switch (type) {
            case 'R':
            /* blank */
            strcpy(op, inst_list[idx].op); //해당 idx 안에 들어있는 op code를 op로 복사

	        if (!strcmp(temp, "jr")) { //rt, rd가 없고  rs만 있음
                fprintf(output, "%s", op);
                temp = strtok(NULL, " \n$\t"); //그 다음꺼 떼어냄 $5 이런거 R 타입 다 앞에 $ 붙어있음
                rs = atoi(temp); //예를 들어 jr $31이면 rs = 31 담김. temp char형이므로 rs라는 int형에 담으려면 atoi로 형변환
                temp = num_to_bits(rs, 5);
                strcpy(inst, temp); //rs
                fprintf(output, "%s", inst); //rs 출력
                fprintf(output, "%s", num_to_bits(rt, 5)); //rt rd shampt 모두 0이어서 15bit 모두 0으로 출력됨
                fprintf(output, "%s", num_to_bits(rd, 5));
                fprintf(output, "%s", num_to_bits(shamt, 5));
                fprintf(output, "%s", inst_list[idx].funct); //funct
		    }

            //일단 jr은 처리 완료됨 제대로 나옴
		
		    else if (!strcmp(temp, "sll") || !strcmp(temp, "srl")) { //rs 없고 shampt가 있음
                //예를들어 sll   $7, $6, 2
                fprintf(output, "%s", op);

                temp = strtok(NULL, " ,$\t"); //그 다음꺼 $7에서 떼어냄
                rd = atoi(temp); //7

                temp = strtok(NULL, " ,$\t"); //6이 담김
                rt = atoi(temp); //char 6 -> int 6

                strcpy(inst, num_to_bits(rt, 5));
                strcat(inst, num_to_bits(rd, 5));

                temp = strtok(NULL, " ,\t\n"); //2가 담김
                //printf("-------shampt %s---\n", temp);
                shamt = atoi(temp); //test 결과 값은 제대로 담긴다
                temp = num_to_bits(shamt, 5);
                //printf("------shampt 이진수 %s----\n", temp);

                fprintf(output, "%s", num_to_bits(rs, 5)); //rs 출력. rs 없어서 00000 출력됨
                fprintf(output, "%s", inst);
                fprintf(output, "%s", num_to_bits(shamt, 5));
                fprintf(output, "%s", inst_list[idx].funct); //funct
		    }
            // sll, srl도 완료됨

		    else { //add sub 등등 나머지는 rs rd rt가 있음
                //rd rs rt
                //ex) and   $11, $11, $0
                fprintf(output, "%s", op);
                temp = strtok(NULL, " \n$\t"); //그 다음꺼 떼어냄 $5 이런거 R 타입 다 앞에 $ 붙어있음
                rd = atoi(temp); //11
                //printf("----%d----\n", rd);

                temp = strtok(NULL, " ,$\t");
                rs = atoi(temp);
                //printf("----%d----\n", rs);

                temp = strtok(NULL, " ,$\t\n");
                rt = atoi(temp);
                //printf("----%d----\n", rt);

                strcpy(inst, num_to_bits(rs, 5));
                strcat(inst, num_to_bits(rt, 5));
                strcat(inst, num_to_bits(rd, 5));
                fprintf(output, "%s", inst);  
                fprintf(output, "%s", num_to_bits(shamt, 5)); //shampt 0이어서 00000으로 출력됨
                //printf("-----%s----\n", inst_list[idx].funct);
                fprintf(output, "%s", inst_list[idx].funct);
	        }

            //드디어 R 타입 처리 끝 ㅠㅠㅠ
            
#if DEBUG
                printf("op:%s rs:$%d rt:$%d rd:$%d shamt:%d funct:%s\n",
                        op, rs, rt, rd, shamt, inst_list[idx].funct);
#endif
            break;

            case 'I':
            /* blank */
            strcpy(op, inst_list[idx].op);

            if (!strcmp(temp, "beq") || !strcmp(temp, "bne")) { //rs와 rt / imm 처리
                //ex bne	$11, $9, lab3
                fprintf(output, "%s", op); //beq 혹은 bne의 op code 출력

                temp = strtok(NULL, " \n$\t"); //그 다음꺼 떼어냄 $11 이런거 $ 떼니까 11만 담김
                rs = atoi(temp); //11

                temp = strtok(NULL, " ,$\t"); //9
                rt = atoi(temp);

                temp = strtok(NULL, " ,\t\n"); //lab3
                //printf("-------%s--여기에 lab3 담겨야함-----\n", temp);
                //imm 처리
                for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
                    if (!strcmp(temp, SYMBOL_TABLE[i].name)) { //lab3 찾았으면 break
                        imm = (SYMBOL_TABLE[i].address - cur_addr) / BYTES_PER_WORD;
                        imm--;
                        break;
                    }
                }
                // printf("-------%i--여기에 lab3 주소 담겨야함-----\n", SYMBOL_TABLE[i].address);  
                // printf("-------%i--cur address\n", cur_addr);
                // printf("-------%i--------------\n", SYMBOL_TABLE[i].address - cur_addr);  
                //imm = ((SYMBOL_TABLE[i].address - cur_addr) / 4) - 1;
                //imm = 0;
                // printf("------%d----\n", imm);
                strcpy(inst, num_to_bits(rs, 5));
                strcat(inst, num_to_bits(rt, 5));
                
                fprintf(output, "%s", inst);
                fprintf(output, num_to_bits(imm, 16));
            }
		
            else if (!strcmp(temp, "lui")) { //rt만 필요한 경우
                fprintf(output, "%s", op);
                //ex) lui     $4, 0x1000
                temp = strtok(NULL, " \n$\t"); //4 떼어내기
                rt = atoi(temp); //4
                strcpy(inst, num_to_bits(rt, 5));

                char *ptr;
                int cnt = 0;

                temp = strtok(NULL, " ,\t\n"); //0x0000 (char)

                ptr = strstr(temp, "x");
                while (ptr != NULL) { //16진수인지 판별하기
                    ptr = strstr(ptr + 1, "x");
                    cnt++;
                }
                
                if (cnt == 0) { //10진수이면 10진수로
                    imm = strtol(temp, NULL, 10);
                }
                else { //16진수면 16진수로로
                    imm = strtol(temp, NULL, 16);
                }

                fprintf(output, "%s", num_to_bits(rs, 5)); //5bit의 00000
                fprintf(output, "%s", inst); //rt
                fprintf(output, num_to_bits(imm, 16));
            } //lui 구현 성공!

            else if (!strcmp(temp, "lw") || !strcmp(temp, "sw")) { //rs와 rt / imm 처리, 괄호 있음
                //ex) lw    $2, 0($4)
                fprintf(output, "%s", op);
                temp = strtok(NULL, " \n$\t"); //2 떼어내기
                rt = atoi(temp); //2

                temp = strtok(NULL, " ,\t()"); //0
                imm = atoi(temp);

                temp = strtok(NULL, " ,\t()$\n"); //4
                rs = atoi(temp);

                strcpy(inst, num_to_bits(rs, 5));
                strcat(inst, num_to_bits(rt, 5));

                fprintf(output, "%s", inst);
                fprintf(output, num_to_bits(imm, 16));
            } //lw sw 구현 성공

            else { //addiu, andi, ori, sltiu
                //ex) addiu    $29, $29, -24
                fprintf(output, "%s", op);

                temp = strtok(NULL, " \n$\t"); //29 떼어내기
                rt = atoi(temp); //29

                temp = strtok(NULL, " ,$\t"); //29
                rs = atoi(temp);

                temp = strtok(NULL, " ,\t\n"); //-24
                char *ptr = strstr(temp, "x");
                int cnt = 0;
                while (ptr != NULL) { //16진수인지 판별하기
                    ptr = strstr(ptr + 1, "x");
                    cnt++;
                }
                
                if (cnt == 0) { //10진수이면 10진수로
                    imm = strtol(temp, NULL, 10);
                }
                else { //16진수면 16진수로로
                    imm = strtol(temp, NULL, 16);
                }

                strcpy(inst, num_to_bits(rs, 5));
                strcat(inst, num_to_bits(rt, 5));

                fprintf(output, "%s", inst);
                fprintf(output, "%s", num_to_bits(imm, 16));
            }

            //i type은 bne beq 빼고 구현 성공,,
#if DEBUG
                printf("op:%s rs:$%d rt:$%d imm:0x%x\n",
                        op, rs, rt, imm);
#endif
                break;		

            case 'J':
            /* blank */
            //ex) j     loop1
            //j 드디어 완료 ㅠㅠ
            strcpy(op, inst_list[idx].op);
            fprintf(output, "%s", op);
            temp = strtok(NULL, " \t\n"); //j 떼어내기

            for (i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
                if (!strcmp(temp, SYMBOL_TABLE[i].name)) {
                    addr = SYMBOL_TABLE[i].address;
                    break;
                }
            }

            addr = addr / BYTES_PER_WORD;
            // printf("=========test address %d======\n", SYMBOL_TABLE[i].address);

            //test 1
            //printf("-------%i------\n", addr);
            // strcpy(temp, num_to_bits(SYMBOL_TABLE[i].address, 32));
            // strcpy(label, temp);
            fprintf(output, "%s", num_to_bits(addr, 26));
            //printf("-----J TYPE TEST %s%s-----------\n", op, temp);
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
        char *temp;
        unsigned int addr;
        char _type[1024] = {0};
        char *ans;
        char *ptr;
        int cnt = 0;

        temp = strtok(line, " \t\n"); //tab, 개행 문자 다음부터 읽어들임 .data 이런거 읽음
        strcpy(_type, temp);
        temp = strtok(NULL, " \t\n"); //data value
        //data value 값이 10진수일 수도 있고 16진수일 수도 있음
        ptr = strstr(temp, "x");
        while (ptr != NULL) { //16진수인지 판별하기
            ptr = strstr(ptr + 1, "x");
            cnt++;
        }
        
        if (cnt == 0) { //10진수이면 10진수로
            addr = strtol(temp, NULL, 10);
        }
        else { //16진수면 16진수로로
            addr = strtol(temp, NULL, 16);
        }
        
        ans = num_to_bits(addr, 32);
        fprintf(output, "%s\n", ans);
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
#if DEBUG
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
    fprintf(output, "%s\n", num_to_bits(text_section_size, 32));
    fprintf(output, "%s\n", num_to_bits(data_section_size, 32));
    //--------------------------------------------------------//
    /* Print .text section */
    record_text_section(output);

    /* Print .data section */
    record_data_section(output);
}

/* Fill the blanks */
void make_symbol_table(FILE *input)
{
    char line[1024] = {0};
    uint32_t address = 0;
    enum section cur_section = MAX_SIZE;

    /* Read each section and put the stream */
    while (fgets(line, 1024, input) != NULL) {
        char *temp;
        char _line[1024] = {0};
        strcpy(_line, line);
        temp = strtok(_line, " \t\n"); //tab, 개행 문자 다음부터 읽어들임
	    //first_pass//
	    //여기서 하는게 temp에 var1 같은거 넣고 address에 var1의 주소값... 이런식으로
        /* Check section type */
        if (!strcmp(temp, ".data")) {
            /* blank */
            //--------------------------//
            //temp = strtok(temp, ":"); //var1, var2, var3
	        address = MEM_DATA_START; //address 초깃값, 한 번 돌 때마다 밑에서 4씩 증가함
	        cur_section = DATA; //밑에 if문을 돌 수 있도록 함
            //printf("%d\n", temp);
            //--------------------------//
            data_seg = tmpfile(); //make temporary file for data segment
            continue;
        }
        else if (!strcmp(temp, ".text")) {
            /* blank */
            //--------------------------//
            //temp = strtok(temp, ":"); 이렇게 하면 .text를 읽게 되네,,
            address = MEM_TEXT_START;
            cur_section = TEXT;
            //--------------------------//
            text_seg = tmpfile(); // make temporary file for text segment
            continue;
        }
	    //여기서 하는게 data segment와 text segment에 .word 1 이런식으로 저장하는거
        /* Put the line into each segment stream */
        if (cur_section == DATA) {
            /* blank */
            int flag_label = 0;
            char temp_label[32];
            symbol_t temp_data;
            //temp = strtok(temp, ":");
            data_section_size += BYTES_PER_WORD; //data section size init: 0
            for (int i = 0; i < strlen(temp); i++) {
                if ( *(temp + i) == ':') { //":"로 하면 char 형식이 아니라서 에러뜸
                    flag_label = 1; //: 들어가면 symbol table에 저장해야 하므로 flag를 세움
                    break;
                }
	        }
            if (flag_label == 1) { //: 들어가면 symbol table로 처리함
                temp = strtok(line, ":"); //이거 _line으로 했을 때는 word 인식 안 됐는데 line으로 바꾸니까 되네 뭐지
                strcpy(temp_label, temp);
                strcpy(temp_data.name, temp_label);
                temp_data.address = address;
                symbol_table_add_entry(temp_data);
                //printf("--=-=-=-=-=-=%s %i-=-=-=-=\n", temp_data.name, temp_data.address);
                temp = strtok(NULL, " \t\n"); //이거 해야지 그 다음거 .var2, .var3 이런거 처리가 됨
            }
            //.word
            fprintf(data_seg, "%s ", temp);
            //.word의 data
            temp = strtok(NULL, " \t\n");
            fprintf(data_seg, "%s\n", temp);

	    //--------------------------------//    
        }
        else if (cur_section == TEXT) {
            /* blank */
            //main:, func1: 같은거 일단 처리
            int flag_label = 0;
            char temp_label[32];
            char temp_reg[20];
            uint32_t temp_address = 0;
            uint16_t upper_address = 0;
            uint16_t lower_address = 0;
            symbol_t temp_text;
            
            for (int i = 0; i < strlen(temp); i++) {
                if (*(temp + i) == ':') {
                    flag_label = 1;
                    break;
                }
	        }
            if (flag_label == 1) { //main, func1... symbol_table에 추가
                temp = strtok(line, ":");
                strcpy(temp_text.name, temp);
                temp_text.address = address;
                symbol_table_add_entry(temp_text);
                temp = strtok(NULL, " \t\n");
                address -= BYTES_PER_WORD;
                //printf("--=-=-=-=-=-=%s %i-=-=-=-=\n", temp_text.name, temp_text.address);
            }
            else { //and, lui...
            
                if (!strcmp(temp, "la")) {
                    temp = strtok(NULL, " \t\n"); //일단 다음거 읽을 준비
                    strcpy(temp_label, "lui\t");
                    fputs(temp_label, text_seg); //lui + 탭까지 출력
                    fprintf(text_seg, "%s ", temp); //공백까지 출력하려고
                    strcpy(temp_reg, temp); //ori 때 필요해서 따로 저장함
                    temp = strtok(NULL, " \t\n");
                    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++) {
                        if (!strcmp(temp, SYMBOL_TABLE[i].name)) {
                            temp_address = SYMBOL_TABLE[i].address;
                            break;
                        }
                    }
                    upper_address = temp_address / 0x10000;
                    lower_address = temp_address;
                    fprintf(text_seg, "0x%04x\n", upper_address);
                    //fputs(temp_label, "\n");
                    if (lower_address != 0x0000) { //0x0000 아닐 때만 ori 출력함
                        strcpy(temp_label, "ori\t");
                        fputs(temp_label, text_seg); //ori + 탭까지 출력
                        fprintf(text_seg, "%s ", temp_reg); //$2
                        fprintf(text_seg, "%s ", temp_reg); //$2
                        fprintf(text_seg, "0x%.4x\n", lower_address);
                        text_section_size += BYTES_PER_WORD;
                        address += BYTES_PER_WORD;
                    }
                    temp = strtok(NULL, "\t\n"); //0x0000일 때는 그냥 다음 줄로 넘어감
                
                }
                else { //text segment의 la가 아닐 때
                    //temp = strtok(NULL, " \t\n");
                    //address -= BYTES_PER_WORD;
                    fprintf(text_seg, "%s\t", temp);
                    temp = strtok(NULL, "\t\n");
                    while (temp != NULL) {
                        fprintf(text_seg, "%s ", temp);
                        temp = strtok(NULL, " \t\n"); //여기서도 일단 다음거 읽을 준비하기
                        //fprintf(text_seg, "%s ", temp);
                    }
                    fputs("\n", text_seg);
                }
                text_section_size += BYTES_PER_WORD;

            }

	    }
        address += BYTES_PER_WORD;
    }
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
    make_symbol_table(input);
    make_binary_file(output);

    fclose(input);
    fclose(output);

    return 0;
}
