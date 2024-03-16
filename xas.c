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
        reg_t joe;
        joe = 0;
        char* buff = malloc(10);
        memcpy(buff, &instruction[2], len);
        joe = atoi(buff);
        free(buff);
        return joe;
    }
}

label_t trans_label(char* instruction, int pc){
    label_t complete;
    complete.key = malloc(strlen(instruction)+1);
    strcpy(complete.key, instruction);
    complete.val = pc;
    return complete;
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
        char* buff = malloc(10);
        memcpy(buff, &instruction[1], len+1);
        joe = atoi(buff);
        free(buff);
        return joe;
    }
}

uint16_t label_lookup(label_t* list, int size, char* key) {
    strncat(key, ":", 30);
    for (int i = 0; i < size; i++){
        if (!strcmp(list[i].key, key)){
            return list[i].val;
        }
    }
    exit(2);
}

reg_t set_reg(FILE* fp, char* store) {
    fscanf(fp, "%s", store);
    reg_t dst = trans_reg(store);
    return dst;
}

reg_t set_val(FILE* fp, char* store) {
    fscanf(fp, "%s", store);
    uint16_t result = trans_val(store);
    return result;
}

uint16_t parse_line(FILE* fp, char* instruction, label_t* list, int c, int pc){
    reg_t dst, src1, src2, base;
    uint16_t result, indirect, offset, imm, cond, jsrflag, op1, op2;
    uint16_t product = 0;
    char store[16];

    if (strstr(instruction, "add") != NULL){
        dst = set_reg(fp, store);
        src1 = set_reg(fp, store);
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

    if (strstr(instruction, "puts") != NULL) {
        if (!strcmp("putsp", instruction)){
            product = emit_trap(TRAP_PUTSP);
        } else {
        product = emit_trap(TRAP_PUTS);
        }
        return product;
    }

    if (strstr(instruction, "getc") != NULL) {
        product = emit_trap(TRAP_GETC);
        return product;
    }

    if (strstr(instruction, "enter") != NULL) {
        product = emit_trap(TRAP_IN);
        return product;
    }

    if (strstr(instruction, "ld") != NULL) {
        dst = set_reg(fp, store);
        if (strstr(instruction, "i") != NULL) {
        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_ldi(dst, offset - pc);
        } else if (strstr(instruction, "r") != NULL) {
        src1 = set_reg(fp, store);
        offset = set_val(fp, store);
        product = emit_ldr(dst, src1, offset);
        } else {
        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_ld(dst, offset - pc);
        }
        return product;
    }

    if (strstr(instruction, "br") != NULL) {
        int n = 0;
        int z = 0;
        int p = 0;
        if (strstr(instruction, "n") != NULL)
            n = 1;
        if (strstr(instruction, "z") != NULL)
            z = 1;
        if (strstr(instruction, "p") != NULL)
            p = 1;

        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_br(n, z, p, offset - pc);
        return product;
    }

    if (strstr(instruction, "halt") != NULL) {
        product = emit_trap(TRAP_HALT);
        return product;
    }

    if (strstr(instruction, "jsr") != NULL) {
        if (strstr(instruction, "rr") != NULL){
        dst = set_reg(fp, store);
        product = emit_jsrr(dst);
        } else {
        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_jsr(offset - pc);
        }
        return product;
    }

    if (strstr(instruction, "jmp") != NULL) {
        dst = set_reg(fp, store);
        product = emit_jmp(dst);
        return product;
    }

    if (strstr(instruction, "lea") != NULL) {
        dst = set_reg(fp, store);
        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_lea(dst, offset-pc);
        return product;
    }

    if (strstr(instruction, "not") != NULL) {
        dst = set_reg(fp, store);
        src1 = set_reg(fp, store);
        product = emit_not(dst, src1);
        return product;
    }

    if (strstr(instruction, "st") != NULL) {
        dst = set_reg(fp, store);
        if (strstr(instruction, "i") != NULL) {
        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_sti(dst, offset - pc);
        } else if (strstr(instruction, "r") != NULL) {
        src1 = set_reg(fp, store);
        offset = set_val(fp, store);
        product = emit_str(dst, src1, offset);
        } else {
        fscanf(fp, "%s", store);
        offset = label_lookup(list, c, store);
        product = emit_st(dst, offset - pc);
        }
        return product;
    }

    exit(2);
}

void free_labels(label_t* list, int size) {
    for (int i = 0; i < size; i++) {
        free(list[i].key);
    }
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
        if (good){
            pc++;
            fgets(line, 60, s_ptr);
        }
    }
    fseek(s_ptr, 0, SEEK_SET);
    pc = 12288;

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
            instruction = parse_line(s_ptr, word, labels, curlabel, pc);
            instruction = htons(instruction);
            fwrite(&instruction, sizeof(uint16_t), 1, a_ptr);
            }
            good = 0;
        }
        if (good) {
            instruction = parse_line(s_ptr, word, labels, curlabel, pc);
            instruction = htons(instruction);
            fwrite(&instruction, sizeof(uint16_t), 1, a_ptr);
        }
    }

    // printf("%d\n", label_lookup(labels, "start:"));
    free_labels(labels, curlabel);
    fclose(s_ptr);
    fclose(a_ptr);
    return 0;
}
