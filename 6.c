
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define MAX_ITERATIONS 1000000

typedef enum {
    STATUS_OK = 0,
    STATUS_ARGUMENTS,
    STATUS_DOMAIN,
    STATUS_OVERFLOW,
    STATUS_CONVERGENCE,
    STATUS_MEMORY
} Status;

static Status is_convex_polygon(size_t n, int *convex, ...)
{
    va_list args;
    double *x = NULL;
    double *y = NULL;
    size_t i;
    int sign = 0;
    int nonzero_cross = 0;
    Status status = STATUS_OK;

    if (convex == NULL || n < 3 ||
        n > SIZE_MAX / sizeof(double)) {
        return STATUS_ARGUMENTS;
    }

    *convex = 0;

    x = malloc(n * sizeof(*x));
    y = malloc(n * sizeof(*y));

    if (x == NULL || y == NULL) {
        free(x);
        free(y);
        return STATUS_MEMORY;
    }

    va_start(args, convex);

    for (i = 0; i < n; ++i) {
        x[i] = va_arg(args, double);
        y[i] = va_arg(args, double);

        if (!isfinite(x[i]) || !isfinite(y[i])) {
            status = STATUS_DOMAIN;
            break;
        }
    }

    va_end(args);

    if (status != STATUS_OK) {
        free(x);
        free(y);
        return status;
    }

    for (i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        size_t k = (i + 2) % n;
        double cross =
            (x[j] - x[i]) * (y[k] - y[j]) -
            (y[j] - y[i]) * (x[k] - x[j]);

        if (!isfinite(cross)) {
            status = STATUS_OVERFLOW;
            break;
        }

        if (cross > 0.0) {
            if (sign < 0) {
                *convex = 0;
                goto polygon_done;
            }
            sign = 1;
            nonzero_cross = 1;
        } else if (cross < 0.0) {
            if (sign > 0) {
                *convex = 0;
                goto polygon_done;
            }
            sign = -1;
            nonzero_cross = 1;
        }
    }

    if (status == STATUS_OK) {
        *convex = nonzero_cross;
    }

polygon_done:
    free(x);
    free(y);
    return status;
}

static Status polynomial_value(double x, int degree,
                               double *result, ...)
{
    va_list args;
    double value;
    double coefficient;
    int i;

    if (result == NULL || degree < 0 || !isfinite(x)) {
        return STATUS_ARGUMENTS;
    }

    va_start(args, result);

    value = va_arg(args, double);

    if (!isfinite(value)) {
        va_end(args);
        return STATUS_DOMAIN;
    }

    for (i = 1; i <= degree; ++i) {
        coefficient = va_arg(args, double);

        if (!isfinite(coefficient)) {
            va_end(args);
            return STATUS_DOMAIN;
        }

        value = value * x + coefficient;

        if (!isfinite(value)) {
            va_end(args);
            return STATUS_OVERFLOW;
        }
    }

    va_end(args);
    *result = value;

    return STATUS_OK;
}

static int digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }

    return -1;
}

static Status parse_base_number(const char *text, int base,
                                uint64_t *number)
{
    uint64_t value = 0;
    size_t i;

    if (text == NULL || text[0] == '\0' ||
        number == NULL || base < 2 || base > 36) {
        return STATUS_ARGUMENTS;
    }

    for (i = 0; text[i] != '\0'; ++i) {
        int digit = digit_value(text[i]);

        if (digit < 0 || digit >= base) {
            return STATUS_DOMAIN;
        }

        if (value > (UINT64_MAX - (uint64_t)digit) /
                    (uint64_t)base) {
            return STATUS_OVERFLOW;
        }

        value = value * (uint64_t)base + (uint64_t)digit;
    }

    *number = value;
    return STATUS_OK;
}

static int is_kaprekar(uint64_t number, int base)
{
    uint64_t square;
    uint64_t power = 1;
    uint64_t right;
    uint64_t left;
    uint64_t temp = number;
    if (number != 0 && number > UINT64_MAX / number) {
        return 0;
    }

    square = number * number;

    do {
        temp /= (uint64_t)base;

        if (temp != 0) {
            if (power > UINT64_MAX / (uint64_t)base) {
                return 0;
            }

            power *= (uint64_t)base;
        }
    } while (temp != 0);

    right = square % power;
    left = square / power;

    if (left > UINT64_MAX - right) {
        return 0;
    }

    return left + right == number;
}

static Status find_kaprekar(int base, size_t count, ...)
{
    va_list args;
    size_t i;
    int found = 0;
    Status status = STATUS_OK;

    if (base < 2 || base > 36 || count == 0) {
        return STATUS_ARGUMENTS;
    }

    va_start(args, count);

    for (i = 0; i < count; ++i) {
        char *text = va_arg(args, char *);
        uint64_t number;

        status = parse_base_number(text, base, &number);

        if (status != STATUS_OK) {
            break;
        }

        if (is_kaprekar(number, base)) {
            printf("%s ", text);
            found = 1;
        }
    }

    va_end(args);

    if (status == STATUS_OK) {
        if (!found) {
            printf("Числа Капрекара не найдены");
        }
        printf("\n");
    }

    return status;
}

static Status geometric_mean(int count, double *result, ...)
{
    va_list args;
    double sum_log = 0.0;
    int i;
    int has_zero = 0;

    if (count <= 0 || result == NULL) {
        return STATUS_ARGUMENTS;
    }

    va_start(args, result);

    for (i = 0; i < count; ++i) {
        double value = va_arg(args, double);

        if (!isfinite(value) || value < 0.0) {
            va_end(args);
            return STATUS_DOMAIN;
        }

        if (value == 0.0) {
            has_zero = 1;
        } else {
            sum_log += log(value);
        }
    }

    va_end(args);

    if (has_zero) {
        *result = 0.0;
    } else {
        *result = exp(sum_log / (double)count);
    }

    if (!isfinite(*result)) {
        return STATUS_OVERFLOW;
    }

    return STATUS_OK;
}

static Status fast_power(double x, int exponent, double *result)
{
    double half;
    double value;
    unsigned int power;

    if (result == NULL || !isfinite(x)) {
        return STATUS_ARGUMENTS;
    }

    if (exponent == 0) {
        *result = 1.0;
        return STATUS_OK;
    }

    if (x == 0.0 && exponent < 0) {
        return STATUS_DOMAIN;
    }

    if (exponent < 0) {
        power = (unsigned int)(-(exponent + 1));
        ++power;
    } else {
        power = (unsigned int)exponent;
    }

    if (power == 1U) {
        value = x;
    } else {
        Status status = fast_power(x, (int)(power / 2U), &half);

        if (status != STATUS_OK) {
            return status;
        }

        value = half * half;

        if ((power % 2U) != 0U) {
            value *= x;
        }
    }

    if (exponent < 0) {
        value = 1.0 / value;
    }

    if (!isfinite(value) ||
        (value == 0.0 && x != 0.0)) {
        return STATUS_OVERFLOW;
    }

    *result = value;
    return STATUS_OK;
}

static Status bisection(double a, double b, double eps,
                        double (*function)(double),
                        double *root)
{
    double fa;
    double fb;
    int iteration;

    if (function == NULL || root == NULL ||
        !isfinite(a) || !isfinite(b) ||
        !isfinite(eps) || eps <= 0.0 || a >= b) {
        return STATUS_ARGUMENTS;
    }

    fa = function(a);
    fb = function(b);

    if (!isfinite(fa) || !isfinite(fb)) {
        return STATUS_DOMAIN;
    }

    if (fa == 0.0) {
        *root = a;
        return STATUS_OK;
    }

    if (fb == 0.0) {
        *root = b;
        return STATUS_OK;
    }

    if ((fa > 0.0 && fb > 0.0) ||
        (fa < 0.0 && fb < 0.0)) {
        return STATUS_DOMAIN;
    }

    for (iteration = 0; iteration < MAX_ITERATIONS; ++iteration) {
        double mid = a + (b - a) / 2.0;
        double fm = function(mid);

        if (!isfinite(fm)) {
            return STATUS_DOMAIN;
        }

        if (fm == 0.0 || (b - a) / 2.0 <= eps) {
            *root = mid;
            return STATUS_OK;
        }

        if ((fa > 0.0 && fm > 0.0) ||
            (fa < 0.0 && fm < 0.0)) {
            a = mid;
            fa = fm;
        } else {
            b = mid;
            fb = fm;
        }
    }

    return STATUS_CONVERGENCE;
}

static double equation_square(double x)
{
    return x * x - 2.0;
}

static double equation_cubic(double x)
{
    return x * x * x - x - 2.0;
}

static double equation_cosine(double x)
{
    return cos(x) - x;
}

static void print_status(Status status)
{
    switch (status) {
        case STATUS_OK:
            break;
        case STATUS_ARGUMENTS:
            printf("Ошибка аргументов.\n");
            break;
        case STATUS_DOMAIN:
            printf("Ошибка области определения.\n");
            break;
        case STATUS_OVERFLOW:
            printf("Переполнение числового типа.\n");
            break;
        case STATUS_CONVERGENCE:
            printf("Не удалось достичь точности.\n");
            break;
        case STATUS_MEMORY:
            printf("Ошибка выделения памяти.\n");
            break;
    }
}

int main(void)
{
    int convex;
    double value;
    double root;
    Status status;

    status = is_convex_polygon(
        4, &convex,
        0.0, 0.0,
        2.0, 0.0,
        2.0, 2.0,
        0.0, 2.0
    );

    if (status == STATUS_OK) {
        printf("1. Многоугольник выпуклый: %s\n",
               convex ? "да" : "нет");
    } else {
        print_status(status);
    }

    status = polynomial_value(2.0, 2, &value,
                              2.0, -3.0, 1.0);

    if (status == STATUS_OK) {
        printf("2. Значение многочлена: %.10g\n", value);
    } else {
        print_status(status);
    }

    printf("3. Числа Капрекара: ");

    status = find_kaprekar(
        10, 8,
        "1", "9", "10", "45",
        "55", "99", "100", "297"
    );

    if (status != STATUS_OK) {
        print_status(status);
    }

    status = geometric_mean(3, &value, 2.0, 8.0, 4.0);

    if (status == STATUS_OK) {
        printf("4. Среднее геометрическое: %.10g\n", value);
    } else {
        print_status(status);
    }

    status = fast_power(2.0, 10, &value);

    if (status == STATUS_OK) {
        printf("5. Степень: %.10g\n", value);
    } else {
        print_status(status);
    }

    status = bisection(1.0, 2.0, 1e-8,
                       equation_square, &root);

    if (status == STATUS_OK) {
        printf("6a. Корень x^2 - 2: %.10g\n", root);
    } else {
        print_status(status);
    }

    status = bisection(1.0, 2.0, 1e-8,
                       equation_cubic, &root);

    if (status == STATUS_OK) {
        printf("6b. Корень x^3 - x - 2: %.10g\n", root);
    } else {
        print_status(status);
    }

    status = bisection(0.0, 1.0, 1e-8,
                       equation_cosine, &root);

    if (status == STATUS_OK) {
        printf("6c. Корень cos(x) - x: %.10g\n", root);
    } else {
        print_status(status);
    }

    return 0;
}