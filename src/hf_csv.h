#ifndef HF_CSV_H
#define HF_CSV_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct HF_CSV_t HF_CSV;

//Creates a new csv struct with given dimensions
//returns a newly allocated HF_CSV struct on success, NULL if any of the dimensions is 0
HF_CSV* hf_csv_create(size_t rows, size_t columns);

//Creates a csv struct from a file
//returns a newly allocated HF_CSV struct on success, NULL if file does not exist
HF_CSV* hf_csv_create_from_file(const char* filename);

//Creates a csv struct from a formatted string. Such string MUST be null-terminated.
//returns a newly allocated HF_CSV struct on success, or NULL if failed to parse
HF_CSV* hf_csv_create_from_string(const char* string);

//Destroys a previously created HF_CSV struct, freeing allocated memory
void hf_csv_destroy(HF_CSV* csv);

//Allocates a new string containing the csv contents
//Returns a valid char* on success, or NULL on failure
char* hf_csv_to_string(HF_CSV* csv);

//Saves csv contents to a file
//Returns true if operation was successful
bool hf_csv_to_file(HF_CSV* csv, const char* filename);

//Search for row containing value in the specified column of csv. Value string MUST be null-terminated
//Returns true if value is found. If so, row index is saved to the provided row pointer
bool hf_csv_find_row(HF_CSV* csv, size_t column, const char* value, size_t* row);

//Search for column containing value in the specified row of csv. Value string MUST be null-terminated
//Returns true if value is found. If so, column index is saved to the provided column pointer
bool hf_csv_find_column(HF_CSV* csv, size_t row, const char* value, size_t* column);

//Gets a value from csv at specified row and column. This pointer may become invalid once the csv struct is modified in any way
//Returns pointer to value if inside bounds of csv file, NULL otherwise
const char* hf_csv_get_value(HF_CSV* csv, size_t row, size_t column);

//Sets a value at specified row and column. The value string MUST be null-terminated
bool hf_csv_set_value(HF_CSV* csv, size_t row, size_t column, const char* value);

//Resizes a csv struct. If size is smaller, values out of bounds will be discarded. If new size is bigger, new values will be empty.
//Returns true if operation was successful. Returns false if csv struct is invalid or any of the newly provided dimensions are 0
bool hf_csv_resize(HF_CSV* csv, size_t rows, size_t columns);

#endif//HF_CSV_H
