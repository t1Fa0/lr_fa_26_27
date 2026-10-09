
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

enum Status {
    STATUS_OK = 0,
    STATUS_INVALID_ARGUMENT,
    STATUS_OVERFLOW,
    STATUS_OUTPUT_ERROR
};

static enum Status parse_number(
    const char *str,
    int *result
) {
    char *end = NULL;
    long value;

    if (str == NULL || result == NULL || str[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    errno = 0;
    value = strtol(str, &end, 10);

    if (errno == ERANGE || end == str || *end != '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (value < 1 || value > INT_MAX) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = (int)value;
    return STATUS_OK;
}

static enum Status print_multiples(const int x) {
    int found = 0;

    for (int i = 1; i <= 100; ++i) {
        if (i % x == 0) {
            if (printf("%d ", i) < 0) {
                return STATUS_OUTPUT_ERROR;
            }
            found = 1;
        }
    }

    if (!found) {
        if (printf("Чисел, кратных %d, в диапазоне от 1 до 100 нет.", x) < 0) {
            return STATUS_OUTPUT_ERROR;
        }
    }

    if (putchar('\n') == EOF) {
        return STATUS_OUTPUT_ERROR;
    }

    return STATUS_OK;
}

static enum Status check_prime(const int x) {
    if (x == 1) {
        if (printf("Число 1 не является ни простым, ни составным.\n") < 0) {
            return STATUS_OUTPUT_ERROR;
        }
        return STATUS_OK;
    }

    for (int i = 2; i <= x / i; ++i) {
        if (x % i == 0) {
            if (printf("%d — составное число.\n", x) < 0) {
                return STATUS_OUTPUT_ERROR;
            }
            return STATUS_OK;
        }
    }

    if (printf("%d — простое число.\n", x) < 0) {
        return STATUS_OUTPUT_ERROR;
    }

    return STATUS_OK;
}

static enum Status print_hex_digits(const int x) {
    static const char digits[] = "0123456789ABCDEF";
    char buffer[sizeof(unsigned int) * CHAR_BIT];
    size_t count = 0;
    unsigned int value = (unsigned int)x;

    do {
        buffer[count++] = digits[value % 16U];
        value /= 16U;
    } while (value != 0U);

    for (size_t i = count; i > 0; --i) {
        if (printf("%c", buffer[i - 1]) < 0) {
            return STATUS_OUTPUT_ERROR;
        }

        if (i > 1 && putchar(' ') == EOF) {
            return STATUS_OUTPUT_ERROR;
        }
    }

    if (putchar('\n') == EOF) {
        return STATUS_OUTPUT_ERROR;
    }

    return STATUS_OK;
}

static enum Status print_power_table(const int x) {
    if (x > 10) {
        fprintf(stderr, "Ошибка: для флага -e число x должно быть от 1 до 10.\n");
        return STATUS_INVALID_ARGUMENT;
    }

    for (int base = 1; base <= 10; ++base) {
        int64_t power = 1;

        for (int exponent = 1; exponent <= x; ++exponent) {
            power *= base;

            if (printf("%" PRId64, power) < 0) {
                return STATUS_OUTPUT_ERROR;
            }

            if (exponent < x && putchar(' ') == EOF) {
                return STATUS_OUTPUT_ERROR;
            }
        }

        if (putchar('\n') == EOF) {
            return STATUS_OUTPUT_ERROR;
        }
    }

    return STATUS_OK;
}

static enum Status print_sum(const int x) {
    const int64_t n = (int64_t)x;
    const int64_t sum = n * (n + 1) / 2;

    if (printf("Сумма чисел от 1 до %d: %" PRId64 "\n", x, sum) < 0) {
        return STATUS_OUTPUT_ERROR;
    }

    return STATUS_OK;
}

static enum Status print_factorial(const int x) {
    if (x > 12) {
        fprintf(stderr,
                "Ошибка: для флага -f число x должно быть от 1 до 12.\n");
        return STATUS_INVALID_ARGUMENT;
    }

    int64_t factorial = 1;

    for (int i = 2; i <= x; ++i) {
        factorial *= i;
    }

    if (printf("%d! = %" PRId64 "\n", x, factorial) < 0) {
        return STATUS_OUTPUT_ERROR;
    }

    return STATUS_OK;
}

static enum Status parse_flag(
    const char *arg,
    char *flag
) {
    if (arg == NULL || flag == NULL ||
        strlen(arg) != 2 ||
        (arg[0] != '-' && arg[0] != '/')) {
        return STATUS_INVALID_ARGUMENT;
    }

    switch (arg[1]) {
        case 'h':
        case 'p':
        case 's':
        case 'e':
        case 'a':
        case 'f':
            *flag = arg[1];
            return STATUS_OK;

        default:
            return STATUS_INVALID_ARGUMENT;
    }
}

int main(int argc, char *argv[]) {
    int x;
    char flag;
    enum Status status;

    if (argc != 3) {
        fprintf(stderr,
                "Использование: %s <x> <-h|-p|-s|-e|-a|-f>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    status = parse_number(argv[1], &x);
    if (status != STATUS_OK) {
        fprintf(stderr,
                "Ошибка: x должно быть натуральным числом в диапазоне от 1 до INT_MAX.\n");
        return EXIT_FAILURE;
    }

    status = parse_flag(argv[2], &flag);
    if (status != STATUS_OK) {
        fprintf(stderr,
                "Ошибка: неизвестный флаг.\n"
                "Допустимые флаги: -h, -p, -s, -e, -a, -f.\n");
        return EXIT_FAILURE;
    }

    switch (flag) {
        case 'h':
            status = print_multiples(x);
            break;

        case 'p':
            status = check_prime(x);
            break;

        case 's':
            status = print_hex_digits(x);
            break;

        case 'e':
            status = print_power_table(x);
            break;

        case 'a':
            status = print_sum(x);
            break;

        case 'f':
            status = print_factorial(x);
            break;

        default:
            status = STATUS_INVALID_ARGUMENT;
            break;
    }

    if (status != STATUS_OK) {
        if (status == STATUS_OUTPUT_ERROR) {
            fprintf(stderr, "Ошибка: не удалось вывести результат.\n");
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}