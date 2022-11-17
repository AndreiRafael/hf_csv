#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HF_CSV_IMPLEMENTATION
#include "hf_csv.h"

#define USE_TEST_ALLOCATOR
//#define TEST_ALLOCATOR_VERBOSE

#ifdef USE_TEST_ALLOCATOR

#ifdef TEST_ALLOCATOR_VERBOSE
#define TEST_ALLOCATOR_RUN(code) code
#else
#define TEST_ALLOCATOR_RUN(code)
#endif

typedef struct Allocation_s {
    void* adress;
    size_t size;
} Allocation;

#define ALLOCATOR_MAX_ALLOCS 2048

typedef struct Allocator_s {
    Allocation* allocations;
    size_t allocation_count;
    size_t full_size;
} Allocator;

static void* allocator_malloc(void* allocator, size_t size) {
    Allocator* a = (Allocator*)allocator;
    if(a->allocation_count >= ALLOCATOR_MAX_ALLOCS) {
        return NULL;
    }

    void* new_alloc = malloc(size);

    if(new_alloc) {
        a->allocations[a->allocation_count] = (Allocation){ .adress = new_alloc, .size = size };
        a->allocation_count++;
        a->full_size += size;
    }

    TEST_ALLOCATOR_RUN(printf("New alloc(%p): %zu\n", new_alloc, size));
    return new_alloc;
}

static void* allocator_realloc(void* allocator, void* mem_ptr, size_t new_size) {
    Allocator* a = (Allocator*)allocator;

    void* new_alloc = realloc(mem_ptr, new_size);

    for(size_t i = 0; i < a->allocation_count; i++) {
        Allocation* allocation = &a->allocations[i];
        if(allocation->adress == mem_ptr) {
            TEST_ALLOCATOR_RUN(printf("Changed alloc(%p->%p): %zu->%zu bytes\n", mem_ptr, new_alloc, allocation->size, new_size);)

            if(new_alloc) {
                a->full_size += new_size;
                a->full_size -= allocation->size;
                *allocation = (Allocation){ .adress = new_alloc, .size = new_size };
            }
            return new_alloc;//early return
        }
    }
    
    if(new_alloc) {
        if(a->allocation_count >= ALLOCATOR_MAX_ALLOCS) {
            free(new_alloc);
            return NULL;
        }

        a->allocations[a->allocation_count] = (Allocation){ .adress = new_alloc, .size = new_size };
        a->allocation_count++;
        a->full_size += new_size;

        TEST_ALLOCATOR_RUN(printf("New alloc from realloc(%p): %zu\n", new_alloc, new_size);)
    }
    return new_alloc;
}

static void allocator_free(void* allocator, void* mem_ptr) {
    Allocator* a = (Allocator*)allocator;

    free(mem_ptr);

    bool found = false;
    for(size_t i = 0; i < a->allocation_count; i++) {
        Allocation* allocation = &a->allocations[i];
        
        if(!found) {
            if(allocation->adress == mem_ptr) {
                found = true;
                a->full_size -= allocation->size;

                TEST_ALLOCATOR_RUN(printf("Freed(%p): %zu\n", mem_ptr, allocation->size);)
            }
        }
        
        if(found && (i + 1) < a->allocation_count) {//copy adjacent allocation
            a->allocations[i] = a->allocations[i + 1];
        }
    }
    if(found) {
        a->allocation_count--;
    }
}

static Allocator test_allocator;

#define TEST_ALLOCATOR (HF_CSV_AllocatorData){ &test_allocator, allocator_malloc, allocator_realloc, allocator_free}
#else
#define TEST_ALLOCATOR NULL
#endif

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

#ifdef USE_TEST_ALLOCATOR
    test_allocator.allocation_count = 0;
    test_allocator.full_size = 0;
    test_allocator.allocations = malloc(sizeof(Allocation) * ALLOCATOR_MAX_ALLOCS);
    memset(test_allocator.allocations, 0, sizeof(Allocation) * ALLOCATOR_MAX_ALLOCS);
    hf_csv_set_allocator_data(TEST_ALLOCATOR);
#endif

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

#ifdef USE_TEST_ALLOCATOR
    printf("%zu allocations not freed: %zu bytes\n", test_allocator.allocation_count, test_allocator.full_size);
    for(size_t i = 0; i < test_allocator.allocation_count; i++) {
        printf("NOT FREED(%p): %zu bytes\n", test_allocator.allocations[i].adress, test_allocator.allocations[i].size);
    }
    free(test_allocator.allocations);
#endif

    return 0;
}
