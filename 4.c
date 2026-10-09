
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ARGUMENTS,
    STATUS_MEMORY,
    STATUS_INPUT_OPEN,
    STATUS_OUTPUT_OPEN,
    STATUS_READ,
    STATUS_WRITE,
    STATUS_CLOSE
} Status;

static int is_ascii_digit(unsigned char c)
{
    return c >= '0' && c <= '9';
}

static int is_latin_letter(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z');
}
static int read_line(FILE *file, unsigned char **buffer,
                     size_t *capacity, size_t *length,
                     int *had_newline)
{
    size_t len = 0;
    int ch;
    unsigned char *new_buffer;

    *had_newline = 0;

    if (*buffer == NULL) {
        *capacity = 128;
        *buffer = malloc(*capacity);

        if (*buffer == NULL) {
            return -1;
        }
    }

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            *had_newline = 1;
            break;
        }

        if (len >= *capacity) {
            size_t new_capacity;

            if (*capacity > (size_t)-1 / 2) {
                return -1;
            }

            new_capacity = *capacity * 2;
            new_buffer = realloc(*buffer, new_capacity);

            if (new_buffer == NULL) {
                return -1;
            }

            *buffer = new_buffer;
            *capacity = new_capacity;
        }

        (*buffer)[len++] = (unsigned char)ch;
    }

    if (ch == EOF && ferror(file)) {
        return -1;
    }

    if (ch == EOF && len == 0) {
        return 0;
    }

    if (*had_newline && len > 0 && (*buffer)[len - 1] == '\r') {
        --len;
    }

    *length = len;
    return 1;
}

static char *make_output_path(const char *input_path)
{
    const char *last_slash = strrchr(input_path, '/');
    const char *last_backslash = strrchr(input_path, '\\');
    const char *separator = last_slash;
    size_t directory_length;
    size_t name_length;
    size_t prefix_length = 4;
    size_t total_length;
    char *output_path;

    if (last_backslash != NULL &&
        (separator == NULL || last_backslash > separator)) {
        separator = last_backslash;
    }

    directory_length = separator
        ? (size_t)(separator - input_path + 1)
        : 0;

    name_length = strlen(input_path) - directory_length;

    if (directory_length > (size_t)-1 - prefix_length ||
        directory_length + prefix_length >
            (size_t)-1 - name_length - 1) {
        return NULL;
    }

    total_length = directory_length + prefix_length
                   + name_length + 1;

    output_path = malloc(total_length);

    if (output_path == NULL) {
        return NULL;
    }

    if (directory_length > 0) {
        memcpy(output_path, input_path, directory_length);
    }

    memcpy(output_path + directory_length, "out_", prefix_length);
    memcpy(output_path + directory_length + prefix_length,
           input_path + directory_length, name_length + 1);

    return output_path;
}

static Status process_line(FILE *output,
                           const unsigned char *line,
                           size_t length,
                           char operation,
                           int had_newline)
{
    size_t i;
    size_t count = 0;

    if (operation == 'd') {
        for (i = 0; i < length; ++i) {
            if (!is_ascii_digit(line[i])) {
                if (fputc(line[i], output) == EOF) {
                    return STATUS_WRITE;
                }
            }
        }
    } else if (operation == 'i') {
        for (i = 0; i < length; ++i) {
            if (is_latin_letter(line[i])) {
                ++count;
            }
        }

        if (fprintf(output, "%zu", count) < 0) {
            return STATUS_WRITE;
        }
    } else if (operation == 's') {
        for (i = 0; i < length; ++i) {
            if (!is_latin_letter(line[i]) &&
                !is_ascii_digit(line[i]) &&
                line[i] != ' ') {
                ++count;
            }
        }

        if (fprintf(output, "%zu", count) < 0) {
            return STATUS_WRITE;
        }
    } else if (operation == 'a') {
        for (i = 0; i < length; ++i) {
            if (is_ascii_digit(line[i])) {
                if (fputc(line[i], output) == EOF) {
                    return STATUS_WRITE;
                }
            } else {
                if (fprintf(output, "%02X",
                            (unsigned int)line[i]) < 0) {
                    return STATUS_WRITE;
                }
            }
        }
    } else {
        return STATUS_ARGUMENTS;
    }

    if (had_newline && fputc('\n', output) == EOF) {
        return STATUS_WRITE;
    }

    return STATUS_OK;
}

static Status process_file(FILE *input, FILE *output, char operation)
{
    unsigned char *line = NULL;
    size_t capacity = 0;
    size_t length = 0;
    int had_newline = 0;
    int read_status;
    Status status = STATUS_OK;

    while ((read_status = read_line(input, &line, &capacity,
                                    &length, &had_newline)) == 1) {
        status = process_line(output, line, length,
                              operation, had_newline);

        if (status != STATUS_OK) {
            break;
        }
    }

    if (read_status == -1 && status == STATUS_OK) {
        status = ferror(input) ? STATUS_READ : STATUS_MEMORY;
    }

    free(line);
    return status;
}

static void print_error(Status status)
{
    switch (status) {
        case STATUS_OK:
            break;
        case STATUS_ARGUMENTS:
            fprintf(stderr, "Ошибка: неверные аргументы командной строки.\n");
            break;
        case STATUS_MEMORY:
            fprintf(stderr, "Ошибка: не удалось выделить память.\n");
            break;
        case STATUS_INPUT_OPEN:
            fprintf(stderr, "Ошибка: не удалось открыть входной файл.\n");
            break;
        case STATUS_OUTPUT_OPEN:
            fprintf(stderr, "Ошибка: не удалось открыть выходной файл.\n");
            break;
        case STATUS_READ:
            fprintf(stderr, "Ошибка: не удалось прочитать входной файл.\n");
            break;
        case STATUS_WRITE:
            fprintf(stderr, "Ошибка: не удалось записать выходной файл.\n");
            break;
        case STATUS_CLOSE:
            fprintf(stderr, "Ошибка: не удалось закрыть файл.\n");
            break;
    }
}

int main(int argc, char *argv[])
{
    const char *flag;
    const char *input_path;
    const char *output_path = NULL;
    char *generated_path = NULL;
    char operation;
    int named_output;
    FILE *input = NULL;
    FILE *output = NULL;
    Status status = STATUS_OK;

    if (argc < 3 || argc > 4) {
        status = STATUS_ARGUMENTS;
        goto finish;
    }

    flag = argv[1];
    input_path = argv[2];

    if (flag == NULL || input_path == NULL ||
        input_path[0] == '\0' ||
        (flag[0] != '-' && flag[0] != '/') ||
        (flag[1] == '\0')) {
        status = STATUS_ARGUMENTS;
        goto finish;
    }

    named_output = (flag[1] == 'n');

    if (named_output) {
        if (flag[2] == '\0' || flag[3] != '\0' || argc != 4 ||
            argv[3][0] == '\0') {
            status = STATUS_ARGUMENTS;
            goto finish;
        }

        operation = flag[2];
        output_path = argv[3];
    } else {
        if (flag[2] != '\0' || argc != 3) {
            status = STATUS_ARGUMENTS;
            goto finish;
        }

        operation = flag[1];

        generated_path = make_output_path(input_path);

        if (generated_path == NULL) {
            status = STATUS_MEMORY;
            goto finish;
        }

        output_path = generated_path;
    }

    if (operation != 'd' && operation != 'i' &&
        operation != 's' && operation != 'a') {
        status = STATUS_ARGUMENTS;
        goto finish;
    }

    if (strcmp(input_path, output_path) == 0) {
        status = STATUS_ARGUMENTS;
        goto finish;
    }

    input = fopen(input_path, "rb");

    if (input == NULL) {
        status = STATUS_INPUT_OPEN;
        goto finish;
    }

    output = fopen(output_path, "wb");

    if (output == NULL) {
        status = STATUS_OUTPUT_OPEN;
        goto finish;
    }

    status = process_file(input, output, operation);

    if (status == STATUS_OK && fflush(output) == EOF) {
        status = STATUS_WRITE;
    }

finish:
    if (output != NULL) {
        if (fclose(output) == EOF && status == STATUS_OK) {
            status = STATUS_CLOSE;
        }
    }

    if (input != NULL) {
        if (fclose(input) == EOF && status == STATUS_OK) {
            status = STATUS_CLOSE;
        }
    }

    free(generated_path);

    if (status != STATUS_OK) {
        print_error(status);
        return 1;
    }

    return 0;
}