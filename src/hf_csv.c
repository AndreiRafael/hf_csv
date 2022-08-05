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

    new_csv->values = malloc(sizeof(char**) * rows);
    if(!new_csv->values) {
        hf_csv_destroy(new_csv);
        return NULL;
    }
    memset(new_csv->values, 0, sizeof(char**) * rows);

    for(size_t row = 0; row < rows; row++) {
        new_csv->values[row] = malloc(sizeof(char*) * columns);
        if(!new_csv->values[row]) {
            hf_csv_destroy(new_csv);
            return NULL;
        }
        memset(new_csv->values[row], 0, sizeof(char*) * columns);

        for(size_t column = 0; column < columns; column++) {
            new_csv->values[row][column] = malloc(sizeof(char));
            if(!new_csv->values[row][column]) {
                hf_csv_destroy(new_csv);
                return NULL;
            }
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
        char* string = malloc(arr_size);
        if(!string) {
            fclose(file);
            return NULL;
        }
        rewind(file);
        
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

//push a char to buffer at index, resize buffer if necessary
static bool push_char_to_buffer(char value, size_t index, char** buffer_ptr, size_t* buffer_size_ptr) {
    while(index >= *buffer_size_ptr) {
        *buffer_size_ptr += 128;
        void* new_memory = realloc(*buffer_ptr, *buffer_size_ptr);
        if(!new_memory) {
            return false;
        }
        *buffer_ptr = new_memory;
    }

    (*buffer_ptr)[index] = value;
    return true;
}

//given string pointer parses next value and inserts it into buffer. On success, modifies string pointer so that it points to the token that terminated the value
static bool parse_value(const char** string_ptr, char** buffer_ptr, size_t* buffer_size_ptr) {
    size_t buffer_index = 0;
    const char* char_itr = *string_ptr;

    bool is_quoted = *char_itr == '\"';
    bool in_quotes = is_quoted;
    if(is_quoted) {
        char_itr++;
    }

    while(true) {
        if(in_quotes) {//just read values, check for end of quotes
            if(*char_itr == '\"') {
                char peek = *(char_itr + 1);
                if(peek == '\"') {//double quotes, push '\"'
                    push_char_to_buffer('\"', buffer_index++, buffer_ptr, buffer_size_ptr);
                    char_itr++;
                }
                else {
                    in_quotes = false;
                }
            }
            else {
                push_char_to_buffer(*char_itr, buffer_index++, buffer_ptr, buffer_size_ptr);
            }
        }
        else {
            if(*char_itr == '\r') {//ignore carriage since not relevant(?) for parsing
                char_itr++;
            }

            if(*char_itr == ',' || *char_itr == '\n' || *char_itr == '\0') {//end of value, null-terminate and check for end of string
                if(*char_itr != '\0') {
                    char peek = *(char_itr + 1);
                    if(peek == '\0') {
                        char_itr++;
                    }
                }

                push_char_to_buffer('\0', buffer_index++, buffer_ptr, buffer_size_ptr);
                *string_ptr = char_itr;
                return true;
            }

            if(is_quoted) {//error, values found after end of quote
                return false;
            }
            else if(*char_itr == '\"') {//error, quotes inside non quoted value
                return false;
            }

            //simply push current value
            push_char_to_buffer(*char_itr, buffer_index++, buffer_ptr, buffer_size_ptr);
        }

        char_itr++;
    }
}

HF_CSV* hf_csv_create_from_string(const char* string) {
    if(!string) {
        return NULL;
    }

    size_t buffer_size = 1;
    char* buffer = malloc(buffer_size);
    if(!buffer) {
        return NULL;
    }

    const char* string_itr = string;

    //first-pass, count values and basic error check
    size_t row_count = 0;
    size_t column_count = 0;
    size_t curr_row = 0;
    size_t curr_column = 0; 
    do {
        bool valid = parse_value(&string_itr, &buffer, &buffer_size);
        if(!valid) {
            free(buffer);
            return NULL;
        }

        //only count columns in first row 
        if(row_count == 0) {
            column_count++;
        }

        curr_column++;
        if(*string_itr == '\n' || *string_itr == '\0') {
            if(curr_row != 0 && curr_column != column_count) {//invalid amout of columns
                free(buffer);
                return NULL;
            }

            row_count++;
            curr_row++;
            curr_column = 0;
            if(*string_itr == '\0') {
                break;
            }
        }

        string_itr++;
    } while(true);
    
    string_itr = string;
    curr_row = 0;
    curr_column = 0;

    //start with a 1x1 table, resize as needed
    HF_CSV* new_csv = hf_csv_create(row_count, column_count);
    do {
        parse_value(&string_itr, &buffer, &buffer_size);

        if(!hf_csv_set_value(new_csv, curr_row, curr_column, buffer)) {//likely allocation error
            hf_csv_destroy(new_csv);
            free(buffer);
            return NULL;
        }

        curr_column++;
        if(*string_itr == '\n') {
            curr_row++;
            curr_column = 0;
        }

        if(*string_itr == '\0') {
            break;
        }

        string_itr++;
    } while(true);
    
    free(buffer);
    return new_csv;
}

void hf_csv_destroy(HF_CSV* csv) {
    if(!csv) {
        return;
    }

    if(csv->values) {
        for(size_t row = 0; row < csv->rows; row++) {
            if(csv->values[row]) {
                for(size_t column = 0; column < csv->columns; column++) {
                    if(csv->values[row][column]) {
                        free(csv->values[row][column]);
                    }
                }
                free(csv->values[row]);
            }
        }
        free(csv->values);
    }
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
    if(!csv || row >= csv->rows || column >= csv->columns) {
        return NULL;
    }

    return csv->values[row][column];
}

bool hf_csv_set_value(HF_CSV* csv, size_t row, size_t column, const char* value) {
    if(!csv || !value || row >= csv->rows || column >= csv->columns) {
        return false;
    }

    size_t new_size = strlen(value) + 1;
    void* new_str = realloc(csv->values[row][column], sizeof(char) * new_size);
    if(!new_str) {
        return false;
    }
    csv->values[row][column] = new_str;
    memcpy(csv->values[row][column], value, new_size);

    return true;
}

bool hf_csv_get_size(HF_CSV* csv, size_t* rows, size_t* columns) {
    if(!csv) {
        return false;
    }

    if(rows) {
        *rows = csv->rows;
    }
    if(columns) {
        *columns = csv->columns;
    }
    return true;
}

bool hf_csv_resize(HF_CSV* csv, size_t rows, size_t columns) {
    if(!csv || rows == 0 || columns == 0 || (rows == csv->rows && columns == csv->columns)) {
        return false;
    }

    //create a new csv to temporarily store values
    HF_CSV* new_csv = hf_csv_create(rows, columns);
    if(!new_csv) {
        return false;
    }

    size_t min_rows = csv->rows > rows ? rows : csv->rows;
    size_t min_columns = csv->columns > columns ? columns : csv->columns;

    //copy each value into new csv
    for(size_t row = 0; row < min_rows; row++) {
        for(size_t column = 0; column < min_columns; column++) {
            bool res = hf_csv_set_value(new_csv, row, column, hf_csv_get_value(csv, row, column));
            if(!res) {
                hf_csv_destroy(new_csv);
                return false;
            }
        }
    }

    //swap values between csv structs and free temp
    HF_CSV temp_csv = *csv;
    *csv = *new_csv;
    *new_csv = temp_csv;
    hf_csv_destroy(new_csv);

    return true;
}
