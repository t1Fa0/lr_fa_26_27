
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

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

static int read_token(FILE *file, char **token, size_t *capacity)
{
    size_t length = 0;
    int ch;
    char *new_token;

    if (*token == NULL) {
        *capacity = 64;
        *token = malloc(*capacity);

        if (*token == NULL) {
            return -1;
        }
    }
    do {
        ch = fgetc(file);

        if (ch == EOF) {
            return ferror(file) ? -1 : 0;
        }
    } while (isspace((unsigned char)ch));
    do {
        if (length >= *capacity - 1) {
            size_t new_capacity;
            
            if (*capacity > SIZE_MAX / 2) {
                return -1;
            }

            new_capacity = *capacity * 2;
            new_token = realloc(*token, new_capacity);

            if (new_token == NULL) {
                return -1;
            }

            *token = new_token;
            *capacity = new_capacity;
        }

        (*token)[length++] = (char)(unsigned char)ch;
        ch = fgetc(file);

    } while (ch != EOF && !isspace((unsigned char)ch));

    if (ch == EOF && ferror(file)) {
        return -1;
    }

    (*token)[length] = '\0';
    return 1;
}

static Status write_separator(FILE *output, int *first_token)
{
    if (!*first_token && fputc(' ', output) == EOF) {
        return STATUS_WRITE;
    }

    *first_token = 0;
    return STATUS_OK;
}

static void to_lower_latin(char *token)
{
    size_t i;

    for (i = 0; token[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)token[i];

        if (c >= 'A' && c <= 'Z') {
            token[i] = (char)(c - 'A' + 'a');
        }
    }
}

static Status write_base_code(FILE *output,
                              unsigned char value,
                              unsigned int base)
{
    static const char digits[] = "0123456789ABCDEF";
    char buffer[8];
    size_t length = 0;

    if (base < 2 || base > 16) {
        return STATUS_ARGUMENTS;
    }

    do {
        buffer[length++] = digits[value % base];
        value = (unsigned char)(value / base);
    } while (value != 0);

    while (length > 0) {
        if (fputc(buffer[--length], output) == EOF) {
            return STATUS_WRITE;
        }
    }

    return STATUS_OK;
}

static Status write_encoded_token(FILE *output,
                                  const char *token,
                                  unsigned int base)
{
    size_t i;

    for (i = 0; token[i] != '\0'; ++i) {
        if (i > 0 && fputc(',', output) == EOF) {
            return STATUS_WRITE;
        }

        if (write_base_code(output,
                            (unsigned char)token[i],
                            base) != STATUS_OK) {
            return STATUS_WRITE;
        }
    }

    return STATUS_OK;
}

static Status interleave_files(FILE *file1, FILE *file2,
                               FILE *output)
{
    char *token1 = NULL;
    char *token2 = NULL;
    size_t capacity1 = 0;
    size_t capacity2 = 0;
    int first_token = 1;
    int result1;
    int result2;
    Status status = STATUS_OK;

    for (;;) {
        result1 = read_token(file1, &token1, &capacity1);

        if (result1 < 0) {
            status = ferror(file1) ? STATUS_READ : STATUS_MEMORY;
            break;
        }

        result2 = read_token(file2, &token2, &capacity2);

        if (result2 < 0) {
            status = ferror(file2) ? STATUS_READ : STATUS_MEMORY;
            break;
        }

        if (result1 == 0 && result2 == 0) {
            break;
        }

        if (result1 == 1) {
            status = write_separator(output, &first_token);

            if (status == STATUS_OK &&
                fputs(token1, output) == EOF) {
                status = STATUS_WRITE;
            }

            if (status != STATUS_OK) {
                break;
            }
        }

        if (result2 == 1) {
            status = write_separator(output, &first_token);

            if (status == STATUS_OK &&
                fputs(token2, output) == EOF) {
                status = STATUS_WRITE;
            }

            if (status != STATUS_OK) {
                break;
            }
        }
    }

    free(token1);
    free(token2);
    return status;
}

static Status transform_file(FILE *input, FILE *output)
{
    char *token = NULL;
    size_t capacity = 0;
    size_t token_number = 0;
    int first_token = 1;
    int read_status;
    Status status = STATUS_OK;

    while ((read_status = read_token(input, &token, &capacity)) == 1) {
        ++token_number;

        status = write_separator(output, &first_token);

        if (status != STATUS_OK) {
            break;
        }

        if (token_number % 10 == 0) {
            to_lower_latin(token);

            status = write_encoded_token(output, token, 4);
        } else if (token_number % 2 == 0) {
            to_lower_latin(token);

            if (fputs(token, output) == EOF) {
                status = STATUS_WRITE;
            }
        } else if (token_number % 5 == 0) {
            status = write_encoded_token(output, token, 8);
        } else {
            if (fputs(token, output) == EOF) {
                status = STATUS_WRITE;
            }
        }

        if (status != STATUS_OK) {
            break;
        }
    }

    if (read_status < 0 && status == STATUS_OK) {
        status = ferror(input) ? STATUS_READ : STATUS_MEMORY;
    }

    free(token);
    return status;
}

static void print_error(Status status)
{
    switch (status) {
        case STATUS_OK:
            break;
        case STATUS_ARGUMENTS:
            fprintf(stderr, "Ошибка: неверные аргументы.\n");
            break;
        case STATUS_MEMORY:
            fprintf(stderr, "Ошибка: недостаточно памяти.\n");
            break;
        case STATUS_INPUT_OPEN:
            fprintf(stderr, "Ошибка: не удалось открыть входной файл.\n");
            break;
        case STATUS_OUTPUT_OPEN:
            fprintf(stderr, "Ошибка: не удалось открыть выходной файл.\n");
            break;
        case STATUS_READ:
            fprintf(stderr, "Ошибка чтения файла.\n");
            break;
        case STATUS_WRITE:
            fprintf(stderr, "Ошибка записи файла.\n");
            break;
        case STATUS_CLOSE:
            fprintf(stderr, "Ошибка закрытия файла.\n");
            break;
    }
}

int main(int argc, char *argv[])
{
    FILE *input1 = NULL;
    FILE *input2 = NULL;
    FILE *output = NULL;
    const char *flag;
    const char *output_path;
    Status status = STATUS_OK;

    if (argc < 2 || argv[1] == NULL) {
        status = STATUS_ARGUMENTS;
        goto cleanup;
    }

    flag = argv[1];

    if ((flag[0] != '-' && flag[0] != '/') ||
        flag[1] == '\0' || flag[2] != '\0') {
        status = STATUS_ARGUMENTS;
        goto cleanup;
    }

    if (flag[1] == 'r') {
        if (argc != 5 || argv[2][0] == '\0' ||
            argv[3][0] == '\0' || argv[4][0] == '\0') {
            status = STATUS_ARGUMENTS;
            goto cleanup;
        }

        if (strcmp(argv[2], argv[3]) == 0 ||
            strcmp(argv[2], argv[4]) == 0 ||
            strcmp(argv[3], argv[4]) == 0) {
            status = STATUS_ARGUMENTS;
            goto cleanup;
        }

        input1 = fopen(argv[2], "rb");

        if (input1 == NULL) {
            status = STATUS_INPUT_OPEN;
            goto cleanup;
        }

        input2 = fopen(argv[3], "rb");

        if (input2 == NULL) {
            status = STATUS_INPUT_OPEN;
            goto cleanup;
        }

        output_path = argv[4];
        output = fopen(output_path, "wb");

        if (output == NULL) {
            status = STATUS_OUTPUT_OPEN;
            goto cleanup;
        }

        status = interleave_files(input1, input2, output);

    } else if (flag[1] == 'a') {
        if (argc != 4 || argv[2][0] == '\0' ||
            argv[3][0] == '\0') {
            status = STATUS_ARGUMENTS;
            goto cleanup;
        }

        if (strcmp(argv[2], argv[3]) == 0) {
            status = STATUS_ARGUMENTS;
            goto cleanup;
        }

        input1 = fopen(argv[2], "rb");

        if (input1 == NULL) {
            status = STATUS_INPUT_OPEN;
            goto cleanup;
        }

        output = fopen(argv[3], "wb");

        if (output == NULL) {
            status = STATUS_OUTPUT_OPEN;
            goto cleanup;
        }

        status = transform_file(input1, output);

    } else {
        status = STATUS_ARGUMENTS;
        goto cleanup;
    }

    if (status == STATUS_OK && fflush(output) == EOF) {
        status = STATUS_WRITE;
    }

cleanup:
    if (output != NULL) {
        if (fclose(output) == EOF && status == STATUS_OK) {
            status = STATUS_CLOSE;
        }
    }

    if (input2 != NULL) {
        if (fclose(input2) == EOF && status == STATUS_OK) {
            status = STATUS_CLOSE;
        }
    }

    if (input1 != NULL) {
        if (fclose(input1) == EOF && status == STATUS_OK) {
            status = STATUS_CLOSE;
        }
    }

    if (status != STATUS_OK) {
        print_error(status);
        return 1;
    }

    return 0;
}