
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <math.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGUMENT,
    STATUS_OUTPUT_ERROR
} Status;

static Status parse_double_value(const char *str, double *result)
{
    char *end = NULL;
    double value;

    if (str == NULL || result == NULL || *str == '\0')
        return STATUS_INVALID_ARGUMENT;

    errno = 0;
    value = strtod(str, &end);

    if (errno == ERANGE || end == str || *end != '\0' ||
        !isfinite(value)) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = value;
    return STATUS_OK;
}

static Status parse_integer(const char *str, intmax_t *result)
{
    char *end = NULL;
    intmax_t value;

    if (str == NULL || result == NULL || *str == '\0')
        return STATUS_INVALID_ARGUMENT;

    errno = 0;
    value = strtoimax(str, &end, 10);

    if (errno == ERANGE || end == str || *end != '\0')
        return STATUS_INVALID_ARGUMENT;

    if (value == 0)
        return STATUS_INVALID_ARGUMENT;

    *result = value;
    return STATUS_OK;
}

static Status parse_flag(const char *str, char *flag)
{
    if (str == NULL || flag == NULL ||
        strlen(str) != 2 ||
        (str[0] != '-' && str[0] != '/')) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (str[1] != 'q' && str[1] != 'm' && str[1] != 't')
        return STATUS_INVALID_ARGUMENT;

    *flag = str[1];
    return STATUS_OK;
}

static Status solve_equation(
    const double a,
    const double b,
    const double c,
    const double eps)
{
    if (fabs(a) <= eps) {
        if (fabs(b) <= eps) {
            if (fabs(c) <= eps) {
                printf("Уравнение имеет бесконечно много решений.\n");
            } else {
                printf("Уравнение не имеет решений.\n");
            }

            return STATUS_OK;
        }

        double x = -c / b;

        if (!isfinite(x)) {
            printf("Решение выходит за пределы диапазона double.\n");
            return STATUS_OK;
        }

        printf("Линейное уравнение: x = %.12g\n", x);
        return STATUS_OK;
    }

    double discriminant = b * b - 4.0 * a * c;

    if (!isfinite(discriminant)) {
        printf("Не удалось вычислить дискриминант: переполнение.\n");
        return STATUS_OK;
    }

    if (discriminant < -eps) {
        printf("Действительных корней нет (D = %.12g).\n",
               discriminant);
        return STATUS_OK;
    }

    if (fabs(discriminant) <= eps) {
        double x = -b / (2.0 * a);

        if (!isfinite(x)) {
            printf("Корень выходит за пределы диапазона double.\n");
            return STATUS_OK;
        }

        printf("Один действительный корень: x = %.12g\n", x);
        return STATUS_OK;
    }

    double sqrt_d = sqrt(discriminant);
    double q = -0.5 * (b + copysign(sqrt_d, b));

    double x1;
    double x2;

    if (q == 0.0) {
        x1 = (-b + sqrt_d) / (2.0 * a);
        x2 = (-b - sqrt_d) / (2.0 * a);
    } else {
        x1 = q / a;
        x2 = c / q;
    }

    if (!isfinite(x1) || !isfinite(x2)) {
        printf("Корни выходят за пределы диапазона double.\n");
        return STATUS_OK;
    }

    printf("Два действительных корня: x1 = %.12g, x2 = %.12g\n",
           x1, x2);

    return STATUS_OK;
}

static int same_permutation(
    const double first[3],
    const double second[3])
{
    return first[0] == second[0] &&
           first[1] == second[1] &&
           first[2] == second[2];
}

static Status solve_permutations(
    const double coefficients[3],
    const double eps)
{
    static const int order[6][3] = {
        {0, 1, 2},
        {0, 2, 1},
        {1, 0, 2},
        {1, 2, 0},
        {2, 0, 1},
        {2, 1, 0}
    };

    double seen[6][3];
    size_t seen_count = 0;

    for (size_t i = 0; i < 6; ++i) {
        double a = coefficients[order[i][0]];
        double b = coefficients[order[i][1]];
        double c = coefficients[order[i][2]];

        double current[3] = {a, b, c};
        int duplicate = 0;

        for (size_t j = 0; j < seen_count; ++j) {
            if (same_permutation(current, seen[j])) {
                duplicate = 1;
                break;
            }
        }

        if (duplicate)
            continue;

        seen[seen_count][0] = a;
        seen[seen_count][1] = b;
        seen[seen_count][2] = c;
        ++seen_count;

        printf("\nУравнение: (%.12g)x^2 + (%.12g)x + (%.12g) = 0\n",
               a, b, c);

        Status status = solve_equation(a, b, c, eps);

        if (status != STATUS_OK)
            return status;
    }

    return STATUS_OK;
}

static Status check_divisibility(
    const intmax_t first,
    const intmax_t second)
{
    if (first == INTMAX_MIN && second == -1) {
        printf("%" PRIdMAX " делится на %" PRIdMAX " без остатка.\n",
               first, second);
        return STATUS_OK;
    }

    if (first % second == 0) {
        printf("%" PRIdMAX " делится на %" PRIdMAX " без остатка.\n",
               first, second);
    } else {
        printf("%" PRIdMAX " не делится на %" PRIdMAX " без остатка.\n",
               first, second);
    }

    return STATUS_OK;
}

static Status check_right_triangle(
    double a,
    double b,
    double c,
    const double eps)
{
    if (a <= 0.0 || b <= 0.0 || c <= 0.0) {
        printf("Длины сторон должны быть положительными.\n");
        return STATUS_OK;
    }

    if (a > b) {
        double temp = a;
        a = b;
        b = temp;
    }

    if (b > c) {
        double temp = b;
        b = c;
        c = temp;
    }

    if (a > b) {
        double temp = a;
        a = b;
        b = temp;
    }

    double aa = a * a;
    double bb = b * b;
    double cc = c * c;

    if (!isfinite(aa) || !isfinite(bb) || !isfinite(cc)) {
        printf("Слишком большие стороны: невозможно вычислить квадраты.\n");
        return STATUS_OK;
    }

    if (a + b <= c) {
        printf("Эти числа не могут быть сторонами треугольника.\n");
        return STATUS_OK;
    }

    double difference = fabs((aa + bb) - cc);

    if (difference <= eps) {
        printf("Стороны могут образовывать прямоугольный треугольник.\n");
    } else {
        printf("Стороны не образуют прямоугольный треугольник "
               "с заданной точностью.\n");
    }

    return STATUS_OK;
}

int main(int argc, char *argv[])
{
    char flag;
    Status status;

    if (argc < 2) {
        fprintf(stderr,
                "Использование:\n"
                "  %s -q <epsilon> <a> <b> <c>\n"
                "  %s -m <целое1> <целое2>\n"
                "  %s -t <epsilon> <a> <b> <c>\n",
                argv[0], argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    status = parse_flag(argv[1], &flag);

    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка: неизвестный флаг.\n");
        return EXIT_FAILURE;
    }

    if (flag == 'm') {
        intmax_t first;
        intmax_t second;

        if (argc != 4) {
            fprintf(stderr,
                    "Ошибка: флаг -m требует ровно два целых числа.\n");
            return EXIT_FAILURE;
        }

        if (parse_integer(argv[2], &first) != STATUS_OK ||
            parse_integer(argv[3], &second) != STATUS_OK) {
            fprintf(stderr,
                    "Ошибка: аргументы -m должны быть ненулевыми "
                    "целыми числами в допустимом диапазоне.\n");
            return EXIT_FAILURE;
        }

        status = check_divisibility(first, second);
    } else {
        double eps;
        double values[3];

        if (argc != 6) {
            fprintf(stderr,
                    "Ошибка: флаг -%c требует epsilon и три числа.\n",
                    flag);
            return EXIT_FAILURE;
        }

        if (parse_double_value(argv[2], &eps) != STATUS_OK ||
            eps <= 0.0) {
            fprintf(stderr,
                    "Ошибка: epsilon должна быть конечным числом больше нуля.\n");
            return EXIT_FAILURE;
        }

        for (int i = 0; i < 3; ++i) {
            if (parse_double_value(argv[i + 3], &values[i]) != STATUS_OK) {
                fprintf(stderr,
                        "Ошибка: коэффициенты и длины должны быть "
                        "конечными вещественными числами.\n");
                return EXIT_FAILURE;
            }
        }

        if (flag == 'q') {
            status = solve_permutations(values, eps);
        } else {
            status = check_right_triangle(
                values[0], values[1], values[2], eps);
        }
    }

    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка выполнения операции.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}