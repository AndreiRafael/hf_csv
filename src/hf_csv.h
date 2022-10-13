#ifndef HF_CSV_H
#define HF_CSV_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct HF_CSV_s HF_CSV;

#ifdef __cplusplus
extern "C" {
#endif

//Creates a new csv struct with given dimensions.
//returns a newly allocated HF_CSV struct on success, NULL if any of the dimensions is 0.
HF_CSV* hf_csv_create(size_t rows, size_t columns);

//Creates a csv struct from a file
//returns a newly allocated HF_CSV struct on success, NULL if file does not exist.
HF_CSV* hf_csv_create_from_file(const char* filename);

//Creates a csv struct from a formatted string. Such string MUST be null-terminated.
//returns a newly allocated HF_CSV struct on success, or NULL if failed to parse.
HF_CSV* hf_csv_create_from_string(const char* string);

//Destroys a previously created HF_CSV struct, freeing allocated memory.
void hf_csv_destroy(HF_CSV* csv);

//Allocates a new string containing the csv contents. This pointer should be later freed by the user.
//Returns a valid null-terminated char* on success, or NULL on failure.
char* hf_csv_to_string(HF_CSV* csv);

//Saves csv contents to a file.
//Returns true if operation was successful.
bool hf_csv_to_file(HF_CSV* csv, const char* filename);

//Search for row containing value in the specified column of csv. Value string MUST be null-terminated.
//Returns true if value is found. If so, row index is saved to the provided row pointer.
bool hf_csv_find_row(HF_CSV* csv, size_t column, const char* value, size_t* row);

//Search for column containing value in the specified row of csv. Value string MUST be null-terminated.
//Returns true if value is found. If so, column index is saved to the provided column pointer.
bool hf_csv_find_column(HF_CSV* csv, size_t row, const char* value, size_t* column);

//Gets a value from csv at specified row and column.
//This pointer may become invalid once the csv struct is modified in any way, and its contents should NOT be freed or modified directly.
//Returns pointer to value if inside bounds of csv file, NULL otherwise.
const char* hf_csv_get_value(HF_CSV* csv, size_t row, size_t column);

//Sets a value at specified row and column. The value string MUST be null-terminated.
bool hf_csv_set_value(HF_CSV* csv, size_t row, size_t column, const char* value);

//gets the num of rows and columns of a csv struct
//Returns true if csv is valid, thus also making valid the values stored in the rows and columns pointers.
bool hf_csv_get_size(HF_CSV* csv, size_t* rows, size_t* columns);

//Resizes a csv struct. If size is smaller, values out of bounds will be discarded. If new size is bigger, new values will be empty.
//Returns true if operation was successful. Returns false if csv struct is invalid, size is maintained or any of the newly provided dimensions are 0.
bool hf_csv_resize(HF_CSV* csv, size_t rows, size_t columns);

#ifdef __cplusplus
}
#endif

#endif//HF_CSV_H

#ifdef HF_CSV_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HF_CSV_s {
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

//push a char to buffer at index, resize buffer if necessary. returns false if buffer resize failed
static bool hf_csv_internal_push_char_to_buffer(char value, size_t index, char** buffer_ptr, size_t* buffer_size_ptr) {
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
static bool hf_csv_internal_parse_value(const char** string_ptr, char** buffer_ptr, size_t* buffer_size_ptr) {
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
                    if(!hf_csv_internal_push_char_to_buffer('\"', buffer_index++, buffer_ptr, buffer_size_ptr)) {
                        return false;
                    }
                    char_itr++;
                }
                else {
                    in_quotes = false;
                }
            }
            else {
                if(!hf_csv_internal_push_char_to_buffer(*char_itr, buffer_index++, buffer_ptr, buffer_size_ptr)) {
                    return false;
                }
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

                if(!hf_csv_internal_push_char_to_buffer('\0', buffer_index++, buffer_ptr, buffer_size_ptr)) {
                    return false;
                }
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
            if(!hf_csv_internal_push_char_to_buffer(*char_itr, buffer_index++, buffer_ptr, buffer_size_ptr)) {
                return false;
            }
        }

        char_itr++;
    }
}

HF_CSV* hf_csv_create_from_string(const char* string) {
    if(!string) {
        return NULL;
    }

    size_t buffer_size = 128;
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
        bool valid = hf_csv_internal_parse_value(&string_itr, &buffer, &buffer_size);
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

    //second pass, create csv and fill it with data
    HF_CSV* new_csv = hf_csv_create(row_count, column_count);
    if(!new_csv) {
        free(buffer);
        return NULL;
    }

    do {
        hf_csv_internal_parse_value(&string_itr, &buffer, &buffer_size);

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
        if(row != 0) {//add extra space for \r\n
            len += 2;
        }

        for(size_t column = 0; column < csv->columns; column++) {
            if(column != 0) {//add extra space for comma
                len++;
            }

            const char* value = csv->values[row][column];
            if(!value) {//uninitialized value
                continue;
            }

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
        if(row != 0) {
            out_string[index++] = '\r';
            out_string[index++] = '\n';
        }

        for(size_t column = 0; column < csv->columns; column++) {
            if(column != 0) {
                out_string[index++] = ',';
            }

            const char* value = csv->values[row][column];
            if(!value) {//uninitialized value
                continue;
            }

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

    const char* value = csv->values[row][column];
    if(!value) {
        return "";
    }
    return value;
}

bool hf_csv_set_value(HF_CSV* csv, size_t row, size_t column, const char* value) {
    if(!csv || !value || row >= csv->rows || column >= csv->columns) {
        return false;
    }

    size_t new_size = strlen(value) + 1;
    char* new_str = realloc(csv->values[row][column], sizeof(char) * new_size);
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

#endif//HF_CSV_IMPLEMENTATION
