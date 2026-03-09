#include <stdio.h>

#define MAX_INPUTS 100

typedef struct {
    int line_number;
    int value;
} Input;

typedef struct {
    int line_number;
    int doubled_value;
} IntermediateInput;

typedef struct {
    int doubled_value;
    int line_numbers[MAX_INPUTS];
    int line_number_count;
} Output;

void map(Input *input, IntermediateInput *intermediate_input) {
    intermediate_input->line_number = input->line_number;
    intermediate_input->doubled_value = input->value * 2;
}

void groupByKey(IntermediateInput input, Output output[], int *result_count) {
    for (int i = 0; i < *result_count; i++) {
        if (output[i].doubled_value == input
