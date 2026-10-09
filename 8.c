
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_ARGUMENTS,
    STATUS_MEMORY,
    STATUS_INPUT_OPEN,
    STATUS_OUTPUT_OPEN,
    STATUS_READ,
    STATUS_WRITE,
    STATUS_INVALID_NUMBER,
    STATUS_OVERFLOW,
    STATUS_CLOSE
} Status;

static int digit_value(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return (int)(c - '0');
    }

    if (c >= 'A' && c <= 'Z') {
        return (int)(c - 'A') + 10;
    }

    if (c >= 'a' && c <= 'z') {
        return (int)(c - 'a') + 10;
    }

    return -1;
}

static void to_upper_ascii(char *text)
{
    size_t i;

    for (i = 0; text[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)text[i];

        if (c >= 'a' && c <= 'z') {
            text[i] = (char)(c - 'a' + 'A');
        }
    }
}

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

static Status analyze_number(const char *number,
                             char **normalized,
                             int *base,
                             uint64_t *decimal_value)
{
    size_t length;
    size_t first = 0;
    size_t i;
    int max_digit = 0;
    uint64_t value = 0;
    char *copy;

    if (number == NULL || normalized == NULL ||
        base == NULL || decimal_value == NULL ||
        number[0] == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    length = strlen(number);

    for (i = 0; i < length; ++i) {
        int digit = digit_value((unsigned char)number[i]);

        if (digit < 0 || digit >= 36) {
            return STATUS_INVALID_NUMBER;
        }

        if (digit > max_digit) {
            max_digit = digit;
        }
    }

    *base = max_digit + 1;

    if (*base < 2) {
        *base = 2;
    }

    while (first + 1 < length && number[first] == '0') {
        ++first;
    }

    copy = malloc(length - first + 1);

    if (copy == NULL) {
        return STATUS_MEMORY;
    }

    memcpy(copy, number + first, length - first + 1);
    to_upper_ascii(copy);

    for (i = first; i < length; ++i) {
        int digit = digit_value((unsigned char)number[i]);

        if (value > (UINT64_MAX - (uint64_t)digit) /
                    (uint64_t)(*base)) {
            free(copy);
            return STATUS_OVERFLOW;
        }

        value = value * (uint64_t)(*base) + (uint64_t)digit;
    }

    *normalized = copy;
    *decimal_value = value;

    return STATUS_OK;
}

static Status process_file(FILE *input, FILE *output)
{
    char *token = NULL;
    size_t capacity = 0;
    int read_status;
    Status status = STATUS_OK;

    while ((read_status = read_token(input, &token, &capacity)) == 1) {
        char *normalized = NULL;
        int base;
        uint64_t decimal_value;

        status = analyze_number(token, &normalized,
                                &base, &decimal_value);

        if (status != STATUS_OK) {
            free(normalized);
            break;
        }
        if (fprintf(output, "%s %d %" PRIu64 "\n",
                    normalized, base, decimal_value) < 0) {
            status = STATUS_WRITE;
        }

        free(normalized);

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
            fprintf(stderr,
                    "Ошибка: необходимо указать входной и выходной файлы.\n");
            break;

        case STATUS_MEMORY:
            fprintf(stderr,
                    "Ошибка: не удалось выделить память.\n");
            break;

        case STATUS_INPUT_OPEN:
            fprintf(stderr,
                    "Ошибка: не удалось открыть входной файл.\n");
            break;

        case STATUS_OUTPUT_OPEN:
            fprintf(stderr,
                    "Ошибка: не удалось открыть выходной файл.\n");
            break;

        case STATUS_READ:
            fprintf(stderr,
                    "Ошибка чтения входного файла.\n");
            break;

        case STATUS_WRITE:
            fprintf(stderr,
                    "Ошибка записи выходного файла.\n");
            break;

        case STATUS_INVALID_NUMBER:
            fprintf(stderr,
                    "Ошибка: обнаружена недопустимая цифра в числе.\n");
            break;

        case STATUS_OVERFLOW:
            fprintf(stderr,
                    "Ошибка: десятичное значение превышает UINT64_MAX.\n");
            break;

        case STATUS_CLOSE:
            fprintf(stderr,
                    "Ошибка закрытия файла.\n");
            break;
    }
}

int main(int argc, char *argv[])
{
    FILE *input = NULL;
    FILE *output = NULL;
    Status status = STATUS_OK;
    if (argc != 3 || argv[1][0] == '\0' ||
        argv[2][0] == '\0') {
        status = STATUS_ARGUMENTS;
        goto cleanup;
    }
    if (strcmp(argv[1], argv[2]) == 0) {
        status = STATUS_ARGUMENTS;
        goto cleanup;
    }

    input = fopen(argv[1], "rb");

    if (input == NULL) {
        status = STATUS_INPUT_OPEN;
        goto cleanup;
    }

    output = fopen(argv[2], "w");

    if (output == NULL) {
        status = STATUS_OUTPUT_OPEN;
        goto cleanup;
    }

    status = process_file(input, output);

    if (status == STATUS_OK && fflush(output) == EOF) {
        status = STATUS_WRITE;
    }

cleanup:
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

    if (status != STATUS_OK) {
        print_error(status);
        return 1;
    }

    return 0;
}