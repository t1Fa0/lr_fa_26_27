
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INPUT,
    STATUS_MEMORY,
    STATUS_INVALID_BASE,
    STATUS_INVALID_NUMBER,
    STATUS_OVERFLOW,
    STATUS_NO_NUMBERS
} Status;

static int read_line(char **line, size_t *capacity)
{
    size_t length = 0;
    int ch;
    char *new_line;

    if (*line == NULL) {
        *capacity = 64;
        *line = malloc(*capacity);

        if (*line == NULL) {
            return -1;
        }
    }

    while ((ch = getchar()) != EOF && ch != '\n') {
        if (length >= *capacity - 1) {
            size_t new_capacity;

            if (*capacity > SIZE_MAX / 2) {
                return -1;
            }

            new_capacity = *capacity * 2;
            new_line = realloc(*line, new_capacity);

            if (new_line == NULL) {
                return -1;
            }

            *line = new_line;
            *capacity = new_capacity;
        }

        (*line)[length++] = (char)(unsigned char)ch;
    }

    if (ch == EOF && ferror(stdin)) {
        return -1;
    }

    if (ch == EOF && length == 0) {
        return 0;
    }

    (*line)[length] = '\0';
    return 1;
}

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

static Status parse_number(const char *text,
                           int base,
                           int64_t *result)
{
    size_t i = 0;
    int negative = 0;
    uint64_t magnitude = 0;
    uint64_t limit;

    if (text == NULL || result == NULL || text[0] == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    if (text[i] == '+' || text[i] == '-') {
        negative = (text[i] == '-');
        ++i;
    }

    if (text[i] == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    limit = negative
        ? (uint64_t)INT64_MAX + UINT64_C(1)
        : (uint64_t)INT64_MAX;

    for (; text[i] != '\0'; ++i) {
        int digit = digit_value((unsigned char)text[i]);

        if (digit < 0 || digit >= base) {
            return STATUS_INVALID_NUMBER;
        }

        if (magnitude > (limit - (uint64_t)digit) /
                        (uint64_t)base) {
            return STATUS_OVERFLOW;
        }

        magnitude = magnitude * (uint64_t)base +
                    (uint64_t)digit;
    }

    if (negative) {
        if (magnitude == (uint64_t)INT64_MAX + UINT64_C(1)) {
            *result = INT64_MIN;
        } else {
            *result = -(int64_t)magnitude;
        }
    } else {
        *result = (int64_t)magnitude;
    }

    return STATUS_OK;
}

static uint64_t magnitude_of(int64_t value)
{
    if (value < 0) {
        return (uint64_t)(-(value + 1)) + UINT64_C(1);
    }

    return (uint64_t)value;
}

static Status add_checked(int64_t a, int64_t b, int64_t *result)
{
    if (b > 0 && a > INT64_MAX - b) {
        return STATUS_OVERFLOW;
    }

    if (b < 0 && a < INT64_MIN - b) {
        return STATUS_OVERFLOW;
    }

    *result = a + b;
    return STATUS_OK;
}

static Status format_number(int64_t value,
                            unsigned int base,
                            char *buffer,
                            size_t buffer_size)
{
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char reversed[65];
    size_t length = 0;
    size_t i;
    uint64_t magnitude;
    int negative = value < 0;

    if (base < 2 || base > 36 ||
        buffer == NULL || buffer_size == 0) {
        return STATUS_INVALID_BASE;
    }

    magnitude = magnitude_of(value);

    do {
        reversed[length++] = digits[magnitude % base];
        magnitude /= base;
    } while (magnitude != 0);

    if (negative) {
        reversed[length++] = '-';
    }

    if (length + 1 > buffer_size) {
        return STATUS_MEMORY;
    }

    for (i = 0; i < length; ++i) {
        buffer[i] = reversed[length - 1 - i];
    }

    buffer[length] = '\0';
    return STATUS_OK;
}

static Status print_conversions(const char *title, int64_t value)
{
    static const unsigned int bases[] = {9, 18, 27, 36};
    size_t i;
    char buffer[66];

    printf("%s: %" PRId64 "\n", title, value);

    for (i = 0; i < sizeof(bases) / sizeof(bases[0]); ++i) {
        Status status = format_number(value, bases[i],
                                      buffer, sizeof(buffer));

        if (status != STATUS_OK) {
            return status;
        }

        printf("Основание %u: %s\n", bases[i], buffer);
    }

    return STATUS_OK;
}

static Status parse_base(const char *text, int *base)
{
    char *end;
    long value;

    if (text == NULL || base == NULL || text[0] == '\0') {
        return STATUS_INVALID_BASE;
    }

    value = strtol(text, &end, 10);

    if (end == text || *end != '\0' || value < 2 || value > 36) {
        return STATUS_INVALID_BASE;
    }

    *base = (int)value;
    return STATUS_OK;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int base;
    int read_status;
    size_t count = 0;
    int64_t sum = 0;
    int64_t max_abs_number = 0;
    uint64_t max_abs = 0;
    Status status;

    printf("Введите основание системы счисления (2..36): ");

    read_status = read_line(&line, &capacity);

    if (read_status != 1) {
        free(line);
        fprintf(stderr, "Ошибка чтения основания.\n");
        return 1;
    }

    status = parse_base(line, &base);

    if (status != STATUS_OK) {
        free(line);
        fprintf(stderr, "Ошибка: основание должно быть от 2 до 36.\n");
        return 1;
    }

    printf("Введите числа по одному в строке.\n");
    printf("Для завершения введите Stop.\n");

    for (;;) {
        int64_t value;
        uint64_t magnitude;
        int is_stop;

        printf("> ");
        read_status = read_line(&line, &capacity);

        if (read_status == 0) {
            break;
        }

        if (read_status < 0) {
            status = STATUS_MEMORY;
            goto cleanup;
        }

        {
            char *start = line;
            char *end;

            while (isspace((unsigned char)*start)) {
                ++start;
            }

            end = start + strlen(start);

            while (end > start &&
                   isspace((unsigned char)end[-1])) {
                --end;
            }

            *end = '\0';

            if (start != line) {
                memmove(line, start, (size_t)(end - start) + 1);
            }
        }

        is_stop = (strlen(line) == 4 &&
                   tolower((unsigned char)line[0]) == 's' &&
                   tolower((unsigned char)line[1]) == 't' &&
                   tolower((unsigned char)line[2]) == 'o' &&
                   tolower((unsigned char)line[3]) == 'p');

        if (is_stop) {
            break;
        }

        status = parse_number(line, base, &value);

        if (status != STATUS_OK) {
            fprintf(stderr,
                    "Ошибка: число '%s' недопустимо или переполнено.\n",
                    line);
            continue;
        }

        status = add_checked(sum, value, &sum);

        if (status != STATUS_OK) {
            fprintf(stderr,
                    "Ошибка: сумма выходит за пределы int64_t.\n");
            goto cleanup;
        }

        magnitude = magnitude_of(value);

        if (count == 0 || magnitude > max_abs) {
            max_abs = magnitude;
            max_abs_number = value;
        }

        ++count;
    }

    if (count == 0) {
        status = STATUS_NO_NUMBERS;
        goto cleanup;
    }

    printf("\nКоличество введённых чисел: %zu\n", count);

    status = print_conversions(
        "Число с максимальным модулем", max_abs_number);

    if (status != STATUS_OK) {
        goto cleanup;
    }

    printf("\n");

    status = print_conversions("Сумма чисел", sum);

cleanup:
    free(line);

    if (status == STATUS_NO_NUMBERS) {
        fprintf(stderr, "Ошибка: не введено ни одного числа.\n");
        return 1;
    }

    if (status == STATUS_OVERFLOW) {
        fprintf(stderr, "Ошибка: переполнение при вычислении суммы.\n");
        return 1;
    }

    if (status != STATUS_OK) {
        fprintf(stderr, "Произошла ошибка выполнения.\n");
        return 1;
    }

    return 0;
}