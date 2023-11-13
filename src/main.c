#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    hf_csv_free_string(my_csv_string);

    hf_csv_to_file(my_csv, "./result.csv");

    hf_csv_resize(my_csv, 2, 2);
    my_csv_string = hf_csv_to_string(my_csv);
    printf("\n\nmy_csv_string after resize 1:\n%s", my_csv_string);
    hf_csv_free_string(my_csv_string);

    hf_csv_resize(my_csv, 3, 3);
    my_csv_string = hf_csv_to_string(my_csv);
    printf("\n\nmy_csv_string after resize 2:\n%s", my_csv_string);
    hf_csv_free_string(my_csv_string);
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

    {//optional line feed test
        HF_CSV* line_feed = hf_csv_create_from_file("./res/line_feed.csv");
        assert(line_feed);

        char* line_feed_str = hf_csv_to_string(line_feed);
        printf("\n\nline_feed csv:\n%s\n", line_feed_str);
        hf_csv_free_string(line_feed_str);

        hf_csv_destroy(line_feed);
    }
    {//badly formatted csv tests
        HF_CSV* bad_column_count = hf_csv_create_from_file("./res/bad_column_count.csv");
        assert(!bad_column_count);
        hf_csv_destroy(bad_column_count);

        HF_CSV* bad_quote = hf_csv_create_from_file("./res/bad_quote.csv");
        assert(!bad_quote);
        hf_csv_destroy(bad_quote);
    }
    {//carriage return
        const char* carriage_src = "a,b,c\r\nd,e,f\r\ng,h,i";
        HF_CSV* carriage = hf_csv_create_from_string(carriage_src);
        assert(carriage);

        char* carriage_str = hf_csv_to_string(carriage);
        assert(carriage_str);

        assert(strlen(carriage_src) == strlen(carriage_str));
        hf_csv_free_string(carriage_str);

        hf_csv_destroy(carriage);
    }
    {//non carriage
        const char* line_feed_src = "a,b,c\nd,e,f\ng,h,i";
        HF_CSV* line_feed_csv = hf_csv_create_from_string(line_feed_src);
        assert(line_feed_csv);

        hf_csv_destroy(line_feed_csv);
    }

    return 0;
}
