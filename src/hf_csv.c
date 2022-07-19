#include "hf_csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HF_CSV {
    char*** values;
    size_t rows;
    size_t columns;
};

HF_CSV* hf_csv_create(size_t rows, size_t columns) {
    if(rows == 0 || columns == 0) {
        return NULL;
    }

    HF_CSV* new_csv = malloc(sizeof(HF_CSV));
    if(!new_csv) {//failed alloc
        return NULL;
    }

    new_csv->rows = rows;
    new_csv->columns = columns;

    // TODO: check malloc for errors
    new_csv->values = malloc(sizeof(char**) * rows);
    for(size_t row = 0; row < rows; row++) {
        new_csv->values[row] = malloc(sizeof(char*) * columns);
        for(size_t column = 0; column < columns; column++) {
            new_csv->values[row][column] = malloc(sizeof(char));
            new_csv->values[row][column][0] = '\0';
        }
    }
    return new_csv;
}

HF_CSV* hf_csv_create_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if(file) {
        size_t arr_size = 1;

        //calc file character count
        char c;
        while((c = (char)fgetc(file)) != EOF) {
            if(c == '\0') {//disregard null-terminator values
                continue;
            }
            arr_size++;
        }

        //transform file into a null-terminated string
        rewind(file);
        char* string = malloc(arr_size);
        size_t curr_index = 0;
        while((c = (char)fgetc(file)) != EOF) {
            if(c == '\0') {//disregard null-terminator values
                continue;
            }
            string[curr_index++] = c;
        }
        string[curr_index] = '\0';
        fclose(file);

        HF_CSV* new_csv = hf_csv_create_from_string(string);
        free(string);
        return new_csv;
    }
    return NULL;
}

//aux func, call before assigning to a buffer to guarantee validity
static void buffer_auto_resize(char** buffer, size_t* size, size_t index) {
    while(index >= *size) {
        *size += 128;
        *buffer = realloc(*buffer, *size);
    }
}

//aux func, call before setting the value of a csv entry
static void new_csv_auto_resize(HF_CSV* csv, size_t row_index, size_t column_index) {
    if(column_index >= csv->columns) {
        hf_csv_resize(csv, csv->rows, column_index + 1);
    }
    if(row_index >= csv->rows) {
        hf_csv_resize(csv, row_index + 1, csv->columns);
    }
}

HF_CSV* hf_csv_create_from_string(const char* string) {
    size_t buffer_size = 128;
    size_t buffer_index = 0;
    char* buffer = malloc(buffer_size);

    bool success = true;
    size_t index = 0;

    size_t curr_row = 0;
    size_t curr_column = 0;

    //start with a 1x1 table, resize as needed
    HF_CSV* new_csv = hf_csv_create(1, 1);
    while(success) {
        char c = string[index];

        if(c == '\0') {
            success = success && ((curr_column == 0 && buffer_index == 0) || (curr_column + 1) == new_csv->columns);
            break;
        }
        else if(c == '\n') {//go to next line
            curr_row++;
            curr_column = 0;
            buffer_index = 0;

            index++;
        }
        else if(c == '\"') {//quoted entry, read until end
            while(true) {
                c = string[++index];
                if(c == '\"') {//next must be '\"' or ','/'\n'/'\0'
                    char next_c = string[++index];
                    if(next_c == '\"') {//double quote, push a single '\"'
                        buffer_auto_resize(&buffer, &buffer_size, buffer_index);
                        buffer[buffer_index++] = '\"';
                    }
                    else if(next_c == ',' || next_c == '\n' || next_c == '\0') {//ended quoted entry, push buffer
                        buffer_auto_resize(&buffer, &buffer_size, buffer_index);
                        buffer[buffer_index] = '\0';

                        new_csv_auto_resize(new_csv, curr_row, curr_column);
                        hf_csv_set_value(new_csv, curr_row, curr_column, buffer);
                        buffer_index = 0;
                        if(next_c == ',') {
                            index++;
                            curr_column++;
                        }
                        break;
                    }
                    else {//failure
                        success = false;
                        break;
                    }
                }
                else if(c == '\0') {//failure
                    success = false;
                    break;
                }
                else {//push curr char to buffer
                    buffer_auto_resize(&buffer, &buffer_size, buffer_index);
                    buffer[buffer_index++] = c;
                }
            }

        }//c == '\"'
        else {//normal entry, read until ',' or '\0'
            while(true) {
                c = string[index];
                if(c == ',' || c == '\0' || c == '\n') {//entry end
                    buffer_auto_resize(&buffer, &buffer_size, buffer_index);
                    buffer[buffer_index] = '\0';

                    new_csv_auto_resize(new_csv, curr_row, curr_column);
                    hf_csv_set_value(new_csv, curr_row, curr_column, buffer);
                    buffer_index = 0;
                    if(c == ',') {
                        index++;
                        curr_column++;
                    }
                    break;
                }
                else if(c == '\"') {//entry error
                    success = false;
                    break;
                }
                else {//push char to buffer
                    buffer_auto_resize(&buffer, &buffer_size, buffer_index);
                    buffer[buffer_index++] = c;

                    index++;
                }
            }
        }
    }

    free(buffer);

    if(!success) {
        hf_csv_destroy(new_csv);
        return NULL;
    }

    return new_csv;
}

void hf_csv_destroy(HF_CSV* csv) {
    if(!csv) {
        return;
    }

    for(size_t row = 0; row < csv->rows; row++) {
        for(size_t column = 0; column < csv->columns; column++) {
            free(csv->values[row][column]);
        }
        free(csv->values[row]);
    }
    free(csv->values);
    free(csv);

    return;
}

char* hf_csv_to_string(HF_CSV* csv) {
    if(!csv) {
        return NULL;
    }

    size_t len = 1;
    for(size_t row = 0; row < csv->rows; row++) {
        if(row != 0) {//add extra space for \n
            len++;
        }

        for(size_t column = 0; column < csv->columns; column++) {
            if(column != 0) {//add extra space for comma
                len++;
            }

            const char* value = csv->values[row][column];
            size_t value_len = strlen(value);
            len += value_len;

            //check if quotes are needed
            for(size_t i = 0; i < value_len; i++) {
                if(value[i] == '\n' || value[i] == '\"' || value[i] == ',') {
                    len += 2;
                    break;
                }
            }
            //check for double quotes requirement
            for(size_t i = 0; i < value_len; i++) {
                if(value[i] == '\"') {
                    len++;
                }
            }
        }
    }

    char* out_string = malloc(len);
    if(!out_string) {
        return NULL;
    }

    size_t index = 0;
    for(size_t row = 0; row < csv->rows; row++) {
        if(row != 0) {//add extra space for \n
            out_string[index++] = '\n';
        }

        for(size_t column = 0; column < csv->columns; column++) {
            if(column != 0) {//add extra space for comma
                out_string[index++] = ',';
            }

            const char* value = csv->values[row][column];
            size_t value_len = strlen(value);

            //check if quotes are needed
            bool needs_quotes = false;
            for(size_t i = 0; i < value_len; i++) {
                if(value[i] == '\n' || value[i] == '\"' || value[i] == ',') {
                    needs_quotes = true;
                    break;
                }
            }
            
            //beggining quote
            if(needs_quotes) {
                out_string[index++] = '\"';
            }

            //copy contents
            for(size_t i = 0; i < value_len; i++) {
                char c = value[i];
                out_string[index++] = c;
                if(c == '\"') {//make double quote
                    out_string[index++] = c;
                }
            }

            //ending quotes
            if(needs_quotes) {
                out_string[index++] = '\"';
            }
        }
    }

    out_string[len - 1] = '\0';
    return out_string;
}

bool hf_csv_to_file(HF_CSV* csv, const char* filename) {
    FILE* file = fopen(filename, "w");
    if(!file) {
        return false;
    }

    char* string = hf_csv_to_string(csv);
    if(!string) {
        return false;
    }

    fprintf(file, "%s", string);

    free(string);
    fclose(file);
    return true;
}

bool hf_csv_find_row(HF_CSV* csv, size_t column, const char* value, size_t* row) {
    if(!csv || column >= csv->columns) {
        return false;
    }

    for(size_t r = 0; r < csv->rows; r++) {
        const char* value_to_compare = hf_csv_get_value(csv, r, column);
        if(strcmp(value, value_to_compare) == 0) {
            if(row) {
                *row = r;
            }
            return true;
        }
    }

    return false;
}

bool hf_csv_find_column(HF_CSV* csv, size_t row, const char* value, size_t* column) {
    if(!csv || row >= csv->rows) {
        return false;
    }

    for(size_t c = 0; c < csv->columns; c++) {
        const char* value_to_compare = hf_csv_get_value(csv, row, c);
        if(strcmp(value, value_to_compare) == 0) {
            if(column) {
                *column = c;
            }
            return true;
        }
    }

    return false;
}

const char* hf_csv_get_value(HF_CSV* csv, size_t row, size_t column) {
    if(!csv || row >= csv->columns || column >= csv->columns) {
        return NULL;
    }

    return csv->values[row][column];
}

bool hf_csv_set_value(HF_CSV* csv, size_t row, size_t column, const char* value) {
    if(!csv || !value || row >= csv->rows || column >= csv->columns) {
        return false;
    }

    size_t new_size = strlen(value) + 1;
    csv->values[row][column] = realloc(csv->values[row][column], sizeof(char) * new_size);
    memcpy(csv->values[row][column], value, new_size);

    return true;
}

bool hf_csv_resize(HF_CSV* csv, size_t rows, size_t columns) {
    if(!csv || rows == 0 || columns == 0) {
        return false;
    }    

    //changing row amount
    if(rows > csv->rows) {//expand
        csv->values = realloc(csv->values, sizeof(char**) * rows);
        //alloc columns for each new row
        for(size_t row = csv->rows; row < rows; row++) {
            csv->values[row] = malloc(sizeof(char*) * csv->columns);
            //insert empty data for each new column
            for(size_t column = 0; column < csv->columns; column++) {
                csv->values[row][column] = malloc(sizeof(char));
                csv->values[row][column][0] = '\0';
            }
        }
    }
    else if(rows < csv->rows) {//shrink
        for(size_t row = rows; row < rows; row++) {
            //free all columns from unused rows
            for(size_t column = 0; column < csv->columns; column++) {
                free(csv->values[row][column]);
            }
            free(csv->values[row]);
        }
        csv->values = realloc(csv->values, sizeof(char**) * rows);
    }
    csv->rows = rows;

    //changing column amount
    if(columns > csv->columns) {//expand
        //alloc new columns for all rows
        for(size_t row = 0; row < csv->rows; row++) {
            csv->values[row] = realloc(csv->values[row], sizeof(char*) * columns);
            //insert empty data for each new column
            for(size_t column = csv->columns; column < columns; column++) {
                csv->values[row][column] = malloc(sizeof(char));
                csv->values[row][column][0] = '\0';
            }
        }
    }
    else if(columns < csv->columns) {//shrink
        for(size_t row = 0; row < csv->rows; row++) {
            //free all data from unused columns
            for(size_t column = columns; column < csv->columns; column++) {
                free(csv->values[row][column]);
            }
            csv->values[row] = realloc(csv->values[row], sizeof(char**) * columns);
        }
    }
    csv->columns = columns;

    return true;
}
