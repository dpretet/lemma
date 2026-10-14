#include "lemma/parser.h"
#include "cargs.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(void)
{
    fprintf(stderr, "usage: lemma <source.sv>\n");
}

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

static TSNode first_syntax_error(TSNode node)
{
    if (ts_node_is_error(node) || ts_node_is_missing(node)) {
        return node;
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t index = 0; index < child_count; ++index) {
        TSNode child = first_syntax_error(ts_node_child(node, index));
        if (!ts_node_is_null(child)) {
            return child;
        }
    }

    return (TSNode){0};
}

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

    TSTree *tree = lemma_parse_source(source, source_length);
    free(source);
    if (tree == NULL) {
        fprintf(stderr, "%s: parser could not process the source\n", argv[1]);
        return EXIT_FAILURE;
    }

    TSNode root = ts_tree_root_node(tree);
    if (ts_node_has_error(root)) {
        TSNode error = first_syntax_error(root);
        TSPoint point = ts_node_start_point(error);
        fprintf(stderr, "%s:%u:%u: syntax error (%s)\n",
                source_path, point.row + 1U, point.column + 1U, ts_node_type(error));
        ts_tree_delete(tree);
        return EXIT_FAILURE;
    }

    printf("parsed successfully: %s (%s)\n", source_path, ts_node_type(root));
    ts_tree_delete(tree);
    return EXIT_SUCCESS;
}
