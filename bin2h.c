/*
 *  Source code written by Gabriel Correia
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>

#if defined (__unix__)
#include "getopt.h"
#else
#include <getopt.h>
#endif

char *temp_mem_buffer = NULL;
    size_t temp_mem_size = 0;

static FILE *temp_file,
    *input_file,
    *output_file,
    *temp_memory;

static const char *input_filename, 
    *output_filename, 
    *symbol_name;

static const char program_name[] = "bin2h";
static const char program_version[] = "0.0.1";

#if defined (__unix__)
static const char *stdin_path = "/dev/stdin";
#endif

static const char *header_prologue = 
    "#pragma once\n\n"
    "#if defined (__cplusplus)\n"
    "extern \"C\" {\n"
    "#endif\n";

static const char *header_epilogue = 
    "\n#if defined (__cplusplus)\n"
    "}\n"
    "#endif\n";

static void cleanup (void)
{
    if (input_file)
        fclose (input_file);
    if (output_file)
        fclose (output_file);
    if (temp_memory)
        fclose (temp_memory);
    
    if (input_filename)
        free ((char*)input_filename);
    if (output_filename)
        free ((char*)output_filename);
    if (symbol_name)
        free ((char*)symbol_name);

    if (temp_mem_buffer)
        free (temp_mem_buffer);

}

static void
#if defined (__linux__)
__attribute__ ((noreturn))
#endif
fatal (const char *fmt, ...)
{
    va_list va;
    va_start (va, fmt);
    vfprintf (stderr, fmt, va);
    va_end (va);

    cleanup ();

    exit (0);
}

static void help (void)
{
    cleanup ();

    fprintf (stdout, "Usage of (%s):\n"
        "\t--help\\-h\n"
        "\t\tDisplay this help menu\n"
        "\t--version\\-v\n"
        "\t\tDisplay the program version\n"
        "\t--input\\-i\n"
        "\t\tThe input filename\n"
        "\t--output\\-o\n"
        "\t\tThe output filename\n"
        "\t--symbol-name\\-S\n"
        "\t\tThe symbol name of the output array\n"
        "\t--column-size\\-C\n"
        "\t\tThe column output size\n"
        "\t--skip\\-s\n"
        "\t\tSkip n bytes from the beginning of the file\n"
        "\t--count\\-c\n"
        "\t\tProcess n bytes from the input data\n",
        program_name
    );

    exit (0);

}

static void version (void)
{
    cleanup ();

    fprintf (stdout, "Version: %s\n",
        program_version
    );

    exit (0);
}

/* Returns the minimum value from two factors */
static int inline min (int x, int y)
{
    return x > y ? y : x;
}

static const char* gen (const char *base, const char *strip, 
    const char *head)
{
    char *string = NULL;

    char *tok, *bak;
    /*  Duplicating the string buffer (to preserver the original 
     *  string to not be modified) 
    */
    base = tok = strdup (base);

    while ((tok = strtok_r (tok, strip, &bak))) {
        string = tok;
        tok = NULL;
    }
    
    tok = string;
    assert ((string = calloc (strlen (tok) + strlen (head) + sizeof (char*), sizeof (char))));

    memcpy (string, tok, strlen (tok));
    memcpy (string + strlen (tok), head, strlen (head));

    free ((char*)base);
    return string;

}


int main (int argc, char **argv)
{
#define BLOCK_SIZE 1024
    int c = 0, optindex = 0, block_size = BLOCK_SIZE, col_size = 8;
    unsigned long skip = 0, count = 0;
    unsigned long offset = 0, bread = 0;
    long int reverse_search = 0;
    unsigned char *buffer = NULL;

    char buffer_time[24];

    time_t raw_time;
    struct tm *realtime;
    
    const char *short_options = "hvi:o:S:C:s:c:";
    const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"symbol-name", required_argument, NULL, 'S'},
        {"column-size", required_argument, NULL, 'C'},
        {"skip", required_argument, NULL, 's'},
        {"count", required_argument, NULL, 'c'},
        {}
    };

    temp_file = input_file = output_file = NULL;
    input_filename = output_filename = symbol_name = NULL;

    while ((c = getopt_long (argc, argv, short_options, long_options, 
        &optindex)) != -1)
        switch (c) {
        case 'h':
            help ();
            break;
        case 'v':
            version ();
            break;
        case 'i':
            input_filename = strdup (optarg);
            break;
        case 'o':
            output_filename = strdup (optarg);
            break;
        case 'S':
            symbol_name = strdup (optarg);
            break;
        case 'C':
            col_size = strtoul (optarg, NULL, 0);
            break;
        case 's':
            skip = strtoul (optarg, NULL, 0);
            break;
        case 'c':
            count = strtoul (optarg, NULL, 0);
            break;
        }

    if ((!input_filename) && optind)
        if ((argc - 1) >= optind)
            if (argv[optind])
                input_filename = strdup (argv[optind]);
    if (col_size % 2)
        fatal ("Error: Incorrect column size, must be a power of 2. e.g: 2, 4, 6, 8...\n");

    if (!input_filename) {
#if defined (__unix__)
        if (!output_filename)
            output_filename = strdup ("out.h");
        if (!symbol_name)
            symbol_name = strdup ("stdin");
        input_filename = strdup (stdin_path);
        input_file = stdin;
#else
        fatal ("Error: Ain't any input filename\n");
#endif
    }

    if (!output_filename)
        /*  Attempting to generate a output filename from 
         *  the input filename 
        */
        output_filename = gen (input_filename, ".,_", ".h");
    if (!symbol_name)
        symbol_name = gen (input_filename, ".,_", "");
    
    if (!input_file)
        if (!(input_file = fopen (input_filename, "r")))
            fatal ("Error: Couldn't open the input file: %s\n", input_filename);
        
    if (!(output_file = fopen (output_filename, "w+")))
        fatal ("Error: Couldn't create/overwrite the output file: %s\n", output_filename);

#if defined (__unix__)
    if (strncmp (input_filename, stdin_path, min (strlen (input_filename), strlen (stdin_path))))
#endif
        if (fseek (input_file, skip, SEEK_SET))
            fatal ("Error: Couldn't seek the file %s to %ld position\n", 
                input_filename, skip);

    temp_memory = open_memstream (&temp_mem_buffer, &temp_mem_size);
    fprintf (temp_memory, "= {");

    assert ((buffer = calloc (BLOCK_SIZE, sizeof (unsigned char))));

    do {
        block_size = count ? count > block_size ? block_size : count : BLOCK_SIZE;

        if ((bread = fread (buffer, 1, block_size, input_file)) == -1) 
            break;

        for (unsigned long k = 0; k < bread; k++) {
            if ((!(k % 2) && (!(k % col_size))))
                fprintf (temp_memory, "\n\t");
            fprintf (temp_memory, "0x%02x, ", (unsigned char) buffer[k]);
        }
        
        offset += bread;
        if (count) {
            count -= bread;
            if (count <= 0)
                break;
        }
    } while (bread > 0);
    
    fseek (temp_memory, -2, SEEK_END);
    fprintf (temp_memory, "\n};\n");
    rewind (temp_memory);

    time (&raw_time);
    realtime = localtime (&raw_time);
    strftime (buffer_time, sizeof (buffer_time) - 1, "%T - %D", realtime);

    fprintf (output_file, "/*\tAuto generated header file by '%s' (VERSION: %s)\n *\tat %s\n*/\n\n",
        program_name, program_version, buffer_time);

    fprintf (output_file, "%s\n", header_prologue);
    fprintf (output_file, "unsigned long long %s_size = %ld;\n\n", 
        symbol_name, offset);
    fprintf (output_file, "unsigned char %s[%ld] ", symbol_name, offset);

    while ((bread = fread (buffer, 1, BLOCK_SIZE, temp_memory)) > 0)
        fwrite (buffer, bread, 1, output_file);

    fprintf (output_file, "%s", header_epilogue);

    free (buffer);

    cleanup ();

    return 0;

#undef BLOCK_SIZE
}

