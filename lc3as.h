#pragma once

typedef struct inst {
    int op;
    int dr, sr1, sr2;
    int immediate;
    char *label;
} inst;

typedef struct inst_list {
    inst *head;
    struct inst_list *tail;
} inst_list;

typedef struct label {
    char *label, *stringz;
    int addr;
} label;

// TODO a more efficient label lookup mechanism
typedef struct label_list {
    label *head;
    struct label_list *tail;
} label_list;
