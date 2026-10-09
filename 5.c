
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define MAX_ITERATIONS 10000000
#define MAX_DEPTH 30

typedef enum {
    STATUS_OK = 0,
    STATUS_ARGUMENTS,
    STATUS_NUMBER,
    STATUS_DOMAIN,
    STATUS_CONVERGENCE,
    STATUS_MEMORY,
    STATUS_CLOSE
} Status;

typedef struct {
    double (*function)(double);
    double tolerance;
    int depth_limit;
    int failed;
} IntegrationContext;

static Status parse_double(const char *text, double *value)
{
    char *end;
    double result;

    if (text == NULL || text[0] == '\0') {
        return STATUS_NUMBER;
    }

    errno = 0;
    result = strtod(text, &end);

    if (errno == ERANGE || end == text || *end != '\0' ||
        !isfinite(result)) {
        return STATUS_NUMBER;
    }

    *value = result;
    return STATUS_OK;
}

static double integrand_a(double x)
{
    if (x == 0.0) {
        return 1.0;
    }

    return log1p(x) / x;
}

static double integrand_b(double x)
{
    return exp(-x * x / 2.0);
}

static double integrand_c(double t)
{
    if (t == 0.0) {
        return 0.0;
    }

    return -4.0 * t * log(t);
}

static double integrand_d(double x)
{
    if (x == 0.0) {
        return 1.0;
    }

    return pow(x, x);
}

static double simpson(double a, double b,
                      double fa, double fm, double fb)
{
    return (b - a) * (fa + 4.0 * fm + fb) / 6.0;
}

static double adaptive_simpson(
    IntegrationContext *ctx,
    double a, double b,
    double fa, double fm, double fb,
    double whole, double eps, int depth)
{
    double mid = (a + b) / 2.0;
    double left_mid = (a + mid) / 2.0;
    double right_mid = (mid + b) / 2.0;
    double flm = ctx->function(left_mid);
    double frm = ctx->function(right_mid);
    double left;
    double right;
    double delta;

    if (!isfinite(flm) || !isfinite(frm)) {
        ctx->failed = 1;
        return 0.0;
    }

    left = simpson(a, mid, fa, flm, fm);
    right = simpson(mid, b, fm, frm, fb);
    delta = left + right - whole;

    if (fabs(delta) <= 15.0 * eps) {
        return left + right + delta / 15.0;
    }

    if (depth >= ctx->depth_limit) {
        ctx->failed = 1;
        return 0.0;
    }

    return adaptive_simpson(
               ctx, a, mid, fa, flm, fm,
               left, eps / 2.0, depth + 1)
         + adaptive_simpson(
               ctx, mid, b, fm, frm, fb,
               right, eps / 2.0, depth + 1);
}

static Status integrate(double (*function)(double),
                        double eps, double *result)
{
    IntegrationContext ctx;
    double a = 0.0;
    double b = 1.0;
    double mid = 0.5;
    double fa = function(a);
    double fm = function(mid);
    double fb = function(b);
    double whole;

    ctx.function = function;
    ctx.tolerance = eps;
    ctx.depth_limit = MAX_DEPTH;
    ctx.failed = 0;

    if (!isfinite(fa) || !isfinite(fm) || !isfinite(fb)) {
        return STATUS_DOMAIN;
    }

    whole = simpson(a, b, fa, fm, fb);

    *result = adaptive_simpson(
        &ctx, a, b, fa, fm, fb,
        whole, eps, 0);

    if (ctx.failed || !isfinite(*result)) {
        return STATUS_CONVERGENCE;
    }

    return STATUS_OK;
}

static Status calculate_series(char variant,
                               double x, double eps,
                               double *result)
{
    double sum = 0.0;
    double term;
    double next_term;
    double ratio;
    double tail;
    size_t n;

    if (variant == 'c' && fabs(x) >= 1.0) {
        return STATUS_DOMAIN;
    }

    if (variant == 'd' && fabs(x) > 1.0) {
        return STATUS_DOMAIN;
    }

    if (variant == 'a') {
        term = 1.0;
        sum = term;

        for (n = 0; n < MAX_ITERATIONS; ++n) {
            ratio = fabs(x) / (double)(n + 1);
            next_term = term * x / (double)(n + 1);

            if (!isfinite(next_term)) {
                return STATUS_CONVERGENCE;
            }

            if (ratio < 1.0) {
                tail = fabs(next_term) / (1.0 - ratio);

                if (tail <= eps) {
                    *result = sum;
                    return STATUS_OK;
                }
            }

            sum += next_term;
            term = next_term;

            if (!isfinite(sum)) {
                return STATUS_CONVERGENCE;
            }
        }
    } else if (variant == 'b') {
        term = 1.0;
        sum = term;

        for (n = 1; n < MAX_ITERATIONS; ++n) {
            ratio = pow(x, 3.0) /
                    ((double)(2 * n) * (double)(2 * n - 1));

            next_term = -term * ratio;

            if (!isfinite(next_term)) {
                return STATUS_CONVERGENCE;
            }

            if (fabs(next_term) <= eps) {
                *result = sum;
                return STATUS_OK;
            }

            sum += next_term;
            term = next_term;

            if (!isfinite(sum)) {
                return STATUS_CONVERGENCE;
            }
        }
    } else if (variant == 'c') {
        term = 1.0;
        sum = term;

        for (n = 1; n < MAX_ITERATIONS; ++n) {
            ratio = 9.0 * (double)n * (double)n * x * x
                  / ((double)(3 * n - 1) *
                     (double)(3 * n - 2));

            next_term = term * ratio;

            if (!isfinite(next_term)) {
                return STATUS_CONVERGENCE;
            }

            if (ratio < 1.0) {
                tail = fabs(next_term) * ratio /
                       (1.0 - ratio);

                if (tail <= eps) {
                    *result = sum;
                    return STATUS_OK;
                }
            }

            sum += next_term;
            term = next_term;

            if (!isfinite(sum)) {
                return STATUS_CONVERGENCE;
            }
        }
    } else if (variant == 'd') {
        term = -x * x;
        sum = 0.0;

        for (n = 1; n < MAX_ITERATIONS; ++n) {
            ratio = ((double)(2 * n + 1) /
                     (double)(2 * n + 2)) * x * x;

            next_term = term;

            if (fabs(next_term) <= eps) {
                *result = sum;
                return STATUS_OK;
            }

            sum += next_term;

            term = -term * ratio;

            if (!isfinite(sum) || !isfinite(term)) {
                return STATUS_CONVERGENCE;
            }
        }
    } else {
        return STATUS_ARGUMENTS;
    }

    return STATUS_CONVERGENCE;
}

static void print_error(Status status)
{
    switch (status) {
        case STATUS_OK:
            break;
        case STATUS_ARGUMENTS:
            fprintf(stderr,
                    "Ошибка: неверные аргументы командной строки.\n");
            break;
        case STATUS_NUMBER:
            fprintf(stderr,
                    "Ошибка: неверное вещественное число.\n");
            break;
        case STATUS_DOMAIN:
            fprintf(stderr,
                    "Ошибка: значение находится вне области сходимости.\n");
            break;
        case STATUS_CONVERGENCE:
            fprintf(stderr,
                    "Ошибка: не удалось достичь заданной точности.\n");
            break;
        case STATUS_MEMORY:
            fprintf(stderr, "Ошибка выделения памяти.\n");
            break;
        case STATUS_CLOSE:
            fprintf(stderr, "Ошибка закрытия файла.\n");
            break;
    }
}

int main(int argc, char *argv[])
{
    double x = 0.0;
    double eps;
    double result = 0.0;
    Status status = STATUS_OK;
    FILE *output = NULL;
    int output_error = 0;
    if (argc >= 2 && strcmp(argv[1], "series") == 0) {
        if (argc != 5 || strlen(argv[2]) != 1) {
            status = STATUS_ARGUMENTS;
        } else {
            status = parse_double(argv[3], &x);

            if (status == STATUS_OK) {
                status = parse_double(argv[4], &eps);
            }

            if (status == STATUS_OK && eps <= 0.0) {
                status = STATUS_NUMBER;
            }

            if (status == STATUS_OK) {
                status = calculate_series(
                    argv[2][0], x, eps, &result);
            }
        }
    } else if (argc >= 2 &&
               strcmp(argv[1], "integral") == 0) {
        double (*function)(double) = NULL;

        if (argc != 4 || strlen(argv[2]) != 1) {
            status = STATUS_ARGUMENTS;
        } else {
            status = parse_double(argv[3], &eps);

            if (status == STATUS_OK && eps <= 0.0) {
                status = STATUS_NUMBER;
            }

            if (status == STATUS_OK) {
                switch (argv[2][0]) {
                    case 'a':
                        function = integrand_a;
                        break;
                    case 'b':
                        function = integrand_b;
                        break;
                    case 'c':
                        function = integrand_c;
                        break;
                    case 'd':
                        function = integrand_d;
                        break;
                    default:
                        status = STATUS_ARGUMENTS;
                        break;
                }
            }

            if (status == STATUS_OK) {
                status = integrate(function, eps, &result);
            }
        }
    } else {
        status = STATUS_ARGUMENTS;
    }

    if (status == STATUS_OK) {
        if (printf("%.15g\n", result) < 0) {
            status = STATUS_CLOSE;
        }
    }

    if (status != STATUS_OK) {
        print_error(status);
        return 1;
    }

    return 0;
}