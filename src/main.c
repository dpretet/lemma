#include "lemma/frontend.h"
#include "cargs.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------
// Print the command-line form expected by the parser-stage executable.
//
// @returns: None.
//--------------------------------------------------------
static void print_usage(void)
{
    fprintf(stderr, "usage: lemma <source.sv>\n");
}

//--------------------------------------------------------
// Read a file into caller-owned NUL-terminated storage, within the parser's
// input-length limit.
//
// @path: Path of the file to read.
// @length: Receives the file byte length on success.
// @returns: Caller-owned file contents on success, or NULL on file, size, read,
//     or allocation failure; the caller must free a returned buffer.
//--------------------------------------------------------
static char *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror(path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror(path);
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file);
    if (file_size < 0 || (uintmax_t)file_size > UINT32_MAX ||
        (uintmax_t)file_size >= SIZE_MAX) {
        fprintf(stderr, "%s: file is too large to parse\n", path);
        fclose(file);
        return NULL;
    }

    rewind(file);
    size_t size = (size_t)file_size;
    char *contents = malloc(size + 1U);
    if (contents == NULL) {
        fprintf(stderr, "%s: unable to allocate file buffer\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(contents, 1U, size, file);
    if (bytes_read != size || ferror(file)) {
        fprintf(stderr, "%s: unable to read file\n", path);
        free(contents);
        fclose(file);
        return NULL;
    }

    fclose(file);
    contents[size] = '\0';
    *length = size;
    return contents;
}

//--------------------------------------------------------
// Render a frontend diagnostic for the source file.
//
// @path: Source path used by the CLI.
// @diagnostic: Diagnostic returned by the frontend entry point.
// @returns: None.
//--------------------------------------------------------
static void print_frontend_diagnostic(const char *path,
                                      const lemma_frontend_diagnostic *diagnostic)
{
    const char *code = diagnostic->code != NULL ? diagnostic->code : "frontend.error";
    if (diagnostic->has_span != 0) {
        fprintf(stderr, "%s:%zu:%zu: %s (%s)\n", path,
                diagnostic->span.start_row + 1U,
                diagnostic->span.start_column + 1U,
                diagnostic->message, code);
    } else {
        fprintf(stderr, "%s: %s (%s)\n", path, diagnostic->message, code);
    }
}

//--------------------------------------------------------
// Load one source file and run the frontend through circuit-IR construction.
//
// @argc: Number of command-line arguments.
// @argv: Command-line argument strings.
// @returns: EXIT_SUCCESS when a circuit is constructed, otherwise EXIT_FAILURE.
//--------------------------------------------------------
int main(int argc, char **argv)
{
    cag_option_context arguments;
    cag_option_init(&arguments, NULL, 0U, argc, argv);
    while (cag_option_fetch(&arguments)) {
        cag_option_print_error(&arguments, stderr);
        print_usage();
        return EXIT_FAILURE;
    }

    int source_index = cag_option_get_index(&arguments);
    if (source_index < argc && strcmp(argv[source_index], "--") == 0) {
        ++source_index;
    }
    if (argc - source_index != 1) {
        print_usage();
        return EXIT_FAILURE;
    }
    const char *source_path = argv[source_index];

    size_t source_length = 0U;
    char *source = read_file(source_path, &source_length);
    if (source == NULL) {
        return EXIT_FAILURE;
    }

    lemma_frontend_diagnostic diagnostic = {0};
    lemma_circuit *circuit = lemma_frontend_build(source, source_length, &diagnostic);
    free(source);
    if (circuit == NULL) {
        print_frontend_diagnostic(source_path, &diagnostic);
        return EXIT_FAILURE;
    }

    printf("front end built circuit: %s (%s)\n", source_path, circuit->module_name);
    lemma_circuit_destroy(circuit);
    return EXIT_SUCCESS;
}
