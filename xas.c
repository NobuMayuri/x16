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

typedef struct {
    char *key;
    uint16_t val;
} label_t;

reg_t trans_reg(char* instruction){
    if (instruction[0] != '%' || strlen(instruction) == 1){
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

label_t trans_label(char* instruction, int pc){
    char buff[20];
    memcpy(buff, &instruction[0], strlen(instruction)-1);
    label_t complete;
    complete.key = buff;
    complete.val = pc;
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

uint16_t parse_line(FILE* fp, char* instruction, label_t* list){
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
            product = emit_add_imm(dst, src1, op2);
        }
        return product;
    }

    if (strstr(instruction, "putc") != NULL) {
        product = emit_trap(TRAP_OUT);
        return product;
    }

    if (strstr(instruction, "ld") != NULL) {
        return product;
    }

    exit(2);
}

uint16_t label_lookup(label_t* list, char* key) {
    char buff[20];
    memcpy(buff, &key[0], strlen(key)-1);
    for (int i = 0; i < sizeof(list) / sizeof(list[0]); i++){
        printf("hi");
        if (strcmp(list[i].key, key)){
            return list[i].val;
        }
    }
    exit(2);
}

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
    a_ptr = fopen("a.obj", "wb+");
    uint16_t joe = 12288;
    joe = htons(joe);
    fwrite(&joe, sizeof(uint16_t), 1, a_ptr);
    label_t labels[30];
    int curlabel = 0;
    char line[60];
    char word[30];
    uint16_t instruction;
    int good = 1;
    int pc = 12288;

    // first pass looking for headers
    while (fscanf(s_ptr, "%s", word) != EOF){
        good = 1;
        pc++;
        if (word[0] == '#'){
            fgets(line, 60, s_ptr);
            good = 0;
        }
        if (word[strlen(word)-1] == ':' && good){
            label_t epico = trans_label(word, pc);
            labels[curlabel] = epico;
            curlabel++;
            good = 0;
        }
    }
    fseek(s_ptr, 0, SEEK_SET);

    // second pass
    while (fscanf(s_ptr, "%s", word) != EOF){
        good = 1;
        pc++;
        if (word[0] == '#'){
            fgets(line, 60, s_ptr);
            good = 0;
        }
        if (word[strlen(word)-1] == ':' && good){
            fscanf(s_ptr, "%s", word);
            if (strstr(word, "val") != NULL){
                fscanf(s_ptr, "%s", word);
                uint16_t goob = trans_val(word);
                goob = emit_value(goob);
                goob = htons(goob);
                fwrite(&goob, sizeof(uint16_t), 1, a_ptr);
            } else {
            instruction = parse_line(s_ptr, word, labels);
            instruction = htons(instruction);
            fwrite(&instruction, sizeof(uint16_t), 1, a_ptr);
            }
            good = 0;
        }
        if (good) {
            instruction = parse_line(s_ptr, word, labels);
            instruction = htons(instruction);
            fwrite(&instruction, sizeof(uint16_t), 1, a_ptr);
        }
    }

    // printf("%d\n", label_lookup(labels, "start:"));
    fclose(s_ptr);
    fclose(a_ptr);
    return 0;
}
