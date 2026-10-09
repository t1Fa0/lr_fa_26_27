
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

#define MAX_ITER 10000000
#define MAX_EPS 0.1

enum Status {
    OK = 0,
    INVALID_ARGUMENT,
    NO_CONVERGENCE,
    OVERFLOW
};

typedef enum Status Status;

typedef Status (*CalcFunction)(double, double *, unsigned long *);

static Status parse_epsilon(const char *str, double *eps)
{
    char *end = NULL;
    double value;

    if (str == NULL || eps == NULL || *str == '\0')
        return INVALID_ARGUMENT;

    errno = 0;
    value = strtod(str, &end);

    if (errno == ERANGE || end == str || *end != '\0')
        return INVALID_ARGUMENT;

    if (!isfinite(value) || value <= 0.0 || value > MAX_EPS)
        return INVALID_ARGUMENT;

    *eps = value;
    return OK;
}

static Status e_limit(double eps, double *result,
                      unsigned long *iterations)
{
    double previous = 0.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double current = exp((double)n * log1p(1.0 / (double)n));

        if (n > 1 && fabs(current - previous) <= eps) {
            *result = current;
            *iterations = n;
            return OK;
        }

        previous = current;
    }

    return NO_CONVERGENCE;
}

static Status pi_limit(double eps, double *result,
                       unsigned long *iterations)
{
    double current = 4.0;

    for (unsigned long n = 2; n <= MAX_ITER; ++n) {
        double dn = (double)n;
        current *= 4.0 * dn * (dn - 1.0) /
                   ((2.0 * dn - 1.0) * (2.0 * dn - 1.0));
        if (fabs(current - 3.14159265358979323846) <= eps) {
            *result = current;
            *iterations = n;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status ln2_limit(double eps, double *result,
                        unsigned long *iterations)
{
    double previous = 0.0;
    const double ln2 = log(2.0);

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double dn = (double)n;
        double current = dn * expm1(ln2 / dn);

        if (n > 1 && fabs(current - previous) <= eps) {
            *result = current;
            *iterations = n;
            return OK;
        }

        previous = current;
    }

    return NO_CONVERGENCE;
}

static Status sqrt2_limit(double eps, double *result,
                          unsigned long *iterations)
{
    double x = -0.5;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double next = x - x * x / 2.0 + 1.0;

        if (!isfinite(next))
            return OVERFLOW;

        if (fabs(next - x) <= eps) {
            *result = next;
            *iterations = n;
            return OK;
        }

        x = next;
    }

    return NO_CONVERGENCE;
}

static Status gamma_limit(double eps, double *result,
                          unsigned long *iterations)
{
    double harmonic = 0.0;
    double previous = 0.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        harmonic += 1.0 / (double)n;
        double current = harmonic - log((double)n);

        if (n > 1 && fabs(current - previous) <= eps) {
            *result = current;
            *iterations = n;
            return OK;
        }

        previous = current;
    }

    return NO_CONVERGENCE;
}

static Status e_series(double eps, double *result,
                       unsigned long *iterations)
{
    double sum = 1.0;
    double term = 1.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        term /= (double)n;
        sum += term;

        if (fabs(term) <= eps) {
            *result = sum;
            *iterations = n;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status pi_series(double eps, double *result,
                        unsigned long *iterations)
{
    double sum = 0.0;

    for (unsigned long n = 0; n < MAX_ITER; ++n) {
        double term = 4.0 / (2.0 * (double)n + 1.0);

        if (n % 2 == 0)
            sum += term;
        else
            sum -= term;

        if (term <= eps) {
            *result = sum;
            *iterations = n + 1;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status ln2_series(double eps, double *result,
                         unsigned long *iterations)
{
    double sum = 0.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double term = 1.0 / (double)n;

        if (n % 2 == 1)
            sum += term;
        else
            sum -= term;

        if (1.0 / (double)(n + 1) <= eps) {
            *result = sum;
            *iterations = n;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status sqrt2_series(double eps, double *result,
                           unsigned long *iterations)
{
    double product = 1.0;
    double term = 1.0;

    for (unsigned long k = 1; k <= 1024; ++k) {
        term *= 0.5;
        double factor = exp(log(2.0) * term);
        product *= factor;

        if (fabs(factor - 1.0) <= eps) {
            *result = product;
            *iterations = k;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status gamma_series(double eps, double *result,
                           unsigned long *iterations)
{
    double sum = 0.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double dn = (double)n;
        double term = 1.0 / dn - log1p(1.0 / dn);
        sum += term;
        if (1.0 / (2.0 * dn) <= eps) {
            *result = sum;
            *iterations = n;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status e_equation(double eps, double *result,
                         unsigned long *iterations)
{
    double x = 2.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double next = x - (log(x) - 1.0) / (1.0 / x);

        if (!isfinite(next) || next <= 0.0)
            return OVERFLOW;

        if (fabs(next - x) <= eps) {
            *result = next;
            *iterations = n;
            return OK;
        }

        x = next;
    }

    return NO_CONVERGENCE;
}

static Status pi_equation(double eps, double *result,
                          unsigned long *iterations)
{
    double left = 2.0;
    double right = 4.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double middle = (left + right) / 2.0;

        if (cos(middle / 2.0) > 0.0)
            left = middle;
        else
            right = middle;

        if (right - left <= eps) {
            *result = (left + right) / 2.0;
            *iterations = n;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status ln2_equation(double eps, double *result,
                           unsigned long *iterations)
{
    double left = 0.0;
    double right = 1.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double middle = (left + right) / 2.0;

        if (exp(middle) < 2.0)
            left = middle;
        else
            right = middle;

        if (right - left <= eps) {
            *result = (left + right) / 2.0;
            *iterations = n;
            return OK;
        }
    }

    return NO_CONVERGENCE;
}

static Status sqrt2_equation(double eps, double *result,
                             unsigned long *iterations)
{
    double x = 1.0;

    for (unsigned long n = 1; n <= MAX_ITER; ++n) {
        double next = (x + 2.0 / x) / 2.0;

        if (fabs(next - x) <= eps) {
            *result = next;
            *iterations = n;
            return OK;
        }

        x = next;
    }

    return NO_CONVERGENCE;
}

static int is_prime(unsigned long n)
{
    if (n < 2)
        return 0;

    for (unsigned long d = 2; d <= n / d; ++d) {
        if (n % d == 0)
            return 0;
    }

    return 1;
}

static Status gamma_equation(double eps, double *result,
                             unsigned long *iterations)
{
    double previous = 0.0;
    int has_previous = 0;

    for (unsigned long t = 3; t <= 1000000; t += 1) {
        double product = 1.0;

        for (unsigned long p = 2; p <= t; ++p) {
            if (is_prime(p))
                product *= ((double)p - 1.0) / (double)p;
        }

        double value = log((double)t) * product;

        if (value <= 0.0 || !isfinite(value))
            return OVERFLOW;

        double current = -log(value);

        if (has_previous && fabs(current - previous) <= eps) {
            *result = current;
            *iterations = t;
            return OK;
        }

        previous = current;
        has_previous = 1;
    }

    return NO_CONVERGENCE;
}

static void print_result(const char *name, CalcFunction function,
                         double eps)
{
    double result = 0.0;
    unsigned long iterations = 0;

    Status status = function(eps, &result, &iterations);

    if (status == OK) {
        printf("  %-22s = %.12f (итераций: %lu)\n",
               name, result, iterations);
    } else if (status == NO_CONVERGENCE) {
        printf("  %-22s: не достигнута точность за отведённое число итераций\n",
               name);
    } else {
        printf("  %-22s: ошибка вычисления (код %d)\n",
               name, (int)status);
    }
}

int main(int argc, char *argv[])
{
    double eps;

    if (argc != 2) {
        fprintf(stderr,
                "Использование: %s <epsilon>\n"
                "Пример: %s 0.0001\n",
                argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    if (parse_epsilon(argv[1], &eps) != OK) {
        fprintf(stderr,
                "Ошибка: epsilon должно быть конечным числом "
                "в диапазоне (0, 0.1].\n");
        return EXIT_FAILURE;
    }

    printf("Точность epsilon = %.10g\n", eps);

    printf("\nКонстанта e:\n");
    print_result("Предел", e_limit, eps);
    print_result("Ряд", e_series, eps);
    print_result("Уравнение ln(x)=1", e_equation, eps);

    printf("\nКонстанта pi:\n");
    print_result("Предел", pi_limit, eps);
    print_result("Ряд Лейбница", pi_series, eps);
    print_result("Уравнение cos(x)=-1", pi_equation, eps);

    printf("\nКонстанта ln(2):\n");
    print_result("Предел", ln2_limit, eps);
    print_result("Ряд", ln2_series, eps);
    print_result("Уравнение exp(x)=2", ln2_equation, eps);

    printf("\nКонстанта sqrt(2):\n");
    print_result("Предел", sqrt2_limit, eps);
    print_result("Произведение", sqrt2_series, eps);
    print_result("Уравнение x^2=2", sqrt2_equation, eps);

    printf("\nКонстанта Эйлера gamma:\n");
    print_result("Предел", gamma_limit, eps);
    print_result("Ряд", gamma_series, eps);
    print_result("Произведение простых", gamma_equation, eps);

    return EXIT_SUCCESS;
}