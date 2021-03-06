#include <stdio.h>

#include "hf_csv.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    //test creating a new csv from scratch
    HF_CSV* my_csv = hf_csv_create(3, 3);

    hf_csv_set_value(my_csv, 0, 0, "hello");
    hf_csv_set_value(my_csv, 0, 1, "world");
    hf_csv_set_value(my_csv, 0, 2, "!");

    hf_csv_set_value(my_csv, 1, 0, "hello world");
    hf_csv_set_value(my_csv, 1, 1, "quote: \"This is a quote\"");
    hf_csv_set_value(my_csv, 1, 2, "!");

    hf_csv_set_value(my_csv, 2, 0, "Multi\nLine\nEntry");
    hf_csv_set_value(my_csv, 2, 1, "---");
    hf_csv_set_value(my_csv, 2, 2, "---");

    char* my_csv_string = hf_csv_to_string(my_csv);
    printf("\n\nmy_csv_string pre resize:\n%s", my_csv_string);
    free(my_csv_string);

    hf_csv_to_file(my_csv, "./result.csv");

    hf_csv_resize(my_csv, 2, 2);
    my_csv_string = hf_csv_to_string(my_csv);
    printf("\n\nmy_csv_string after resize 1:\n%s", my_csv_string);
    free(my_csv_string);

    hf_csv_resize(my_csv, 3, 3);
    my_csv_string = hf_csv_to_string(my_csv);
    printf("\n\nmy_csv_string after resize 2:\n%s", my_csv_string);
    free(my_csv_string);
    hf_csv_destroy(my_csv);

    //Localization testing
    HF_CSV* loc_csv = hf_csv_create_from_file("./res/loc.csv");

    size_t hello_row = 0;
    hf_csv_find_row(loc_csv, 0, "HELLO_WORLD", &hello_row);
    size_t multi_row = 0;
    hf_csv_find_row(loc_csv, 0, "MULTI_LINE", &multi_row);

    size_t pt_column = 0;
    hf_csv_find_column(loc_csv, 0, "PT", &pt_column);
    const char* pt_hello = hf_csv_get_value(loc_csv, hello_row, pt_column);
    const char* pt_multi = hf_csv_get_value(loc_csv, multi_row, pt_column);
    printf("\n\nPT\n%s\n%s", pt_hello, pt_multi);

    size_t en_column = 0;
    hf_csv_find_column(loc_csv, 0, "EN", &en_column);
    const char* en_hello = hf_csv_get_value(loc_csv, hello_row, en_column);
    const char* en_multi = hf_csv_get_value(loc_csv, multi_row, en_column);
    printf("\n\nEN\n%s\n%s", en_hello, en_multi);

    //Testing write lang file to disk
    hf_csv_to_file(loc_csv, "./lang_result.csv");
    hf_csv_destroy(loc_csv);

    return 0;
}
