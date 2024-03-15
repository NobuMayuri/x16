#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "instruction.h"


void usage() {
    fprintf(stderr, "Usage: ./xas file");
    exit(1);
}

reg_t trans_reg(char* instruction){
    if (instruction[0] != '%'){
        exit(2);
    } else {
        int len = strlen(instruction)-1;
        if (instruction[len] == ',')
            len = len-1;
        reg_t joe = 0;
        char buff[3];
        memcpy(buff, &instruction[2], len-1);
        joe = atoi(buff);
        return joe;
    }
}

uint16_t trans_val(char* instruction){
    if (instruction[0] != '$'){
        exit(2);
    } else {
        int len = strlen(instruction)-1;
        if (instruction[len] == ','){
            len = len-1;
        }
        uint16_t joe = 0;
        char buff[20];
        memcpy(buff, &instruction[1], len);
        joe = atoi(buff);
        return joe;
    }
}

uint16_t parse_line(FILE* fp, char* instruction){
    reg_t dst, src1, src2, base;
    uint16_t result, indirect, offset, imm, cond, jsrflag, op1, op2;
    uint16_t product = 0;
    char store[16];

    if (strstr(instruction, "add") != NULL){
        fscanf(fp, "%s", store);
        dst = trans_reg(store);
        fscanf(fp, "%s", store);
        src1 = trans_reg(store);
        fscanf(fp, "%s", store);
        if (store[0] == '%'){
            src2 = trans_reg(store);
            product = emit_add_reg(dst, src1, src2);
        } else {
            op2 = trans_val(store);
            product = emit_add_imm(dst, src2, op2);
        }
    }
    return product;
}


typedef struct {
    char *key;
    int val;
} t_header;

int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
    }

    FILE* s_ptr;
    s_ptr = fopen(argv[1], "r");

    if (s_ptr == NULL){
        printf("No file found\n");
        exit(2);
    }

    FILE* a_ptr;
    a_ptr = fopen("a.obj", "w");
    fprintf(a_ptr, "3000 ");

    char line[60];
    char word[30];
    uint16_t instruction;
    int good = 1;
    // first pass looking for headers
    while (fscanf(s_ptr, "%s", word) != EOF){
        good = 1;
        if(word[0] == '#'){
            fgets(line, 60, s_ptr);
            good = 0;
        }
        if(word[strlen(word)-1] == ':' && good){

            good = 0;
        }


    }

    fseek(s_ptr, 0, SEEK_SET);

    // second pass
    while (fscanf(s_ptr, "%s", word) != EOF){
        good = 1;
        if(word[0] == '#'){
            fgets(line, 60, s_ptr);
            good = 0;
        }
        if(word[strlen(word)-1] == ':' && good){
            good = 0;
        }
        if (good) {
            instruction = parse_line(s_ptr, word);
            fprintf(a_ptr, "%x ", instruction);
        }
    }

    fclose(s_ptr);
    fclose(a_ptr);
    return 0;
}
