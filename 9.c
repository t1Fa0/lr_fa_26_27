
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#define ARRAY_SIZE 20
#define MIN_SIZE 10
#define MAX_SIZE 10000
#define MIN_VALUE (-1000)
#define MAX_VALUE 1000

typedef enum {
    STATUS_OK = 0,
    STATUS_ARGUMENTS,
    STATUS_MEMORY,
    STATUS_RANGE
} Status;

static Status parse_int(const char *text, int *result)
{
    char *end;
    long value;

    if (text == NULL || text[0] == '\0' || result == NULL) {
        return STATUS_ARGUMENTS;
    }

    errno = 0;
    value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' ||
        value < -2147483647L - 1L ||
        value > 2147483647L) {
        return STATUS_ARGUMENTS;
    }

    *result = (int)value;
    return STATUS_OK;
}

static int random_int(int a, int b)
{
    return a + rand() % (b - a + 1);
}

static void print_array(const char *name, const int *array, size_t size)
{
    size_t i;

    printf("%s: ", name);

    for (i = 0; i < size; ++i) {
        printf("%d", array[i]);

        if (i + 1 < size) {
            printf(" ");
        }
    }

    printf("\n");
}

static Status task_one(int a, int b)
{
    int array[ARRAY_SIZE];
    size_t min_index = 0;
    size_t max_index = 0;
    size_t i;
    int temp;

    if (a > b) {
        return STATUS_RANGE;
    }

    for (i = 0; i < ARRAY_SIZE; ++i) {
        array[i] = random_int(a, b);
    }

    printf("Задача 1\n");
    print_array("Исходный массив", array, ARRAY_SIZE);

    for (i = 1; i < ARRAY_SIZE; ++i) {
        if (array[i] < array[min_index]) {
            min_index = i;
        }

        if (array[i] > array[max_index]) {
            max_index = i;
        }
    }

    printf("Минимум: %d, индекс: %zu\n",
           array[min_index], min_index);
    printf("Максимум: %d, индекс: %zu\n",
           array[max_index], max_index);

    temp = array[min_index];
    array[min_index] = array[max_index];
    array[max_index] = temp;

    print_array("После обмена", array, ARRAY_SIZE);

    return STATUS_OK;
}

static size_t find_closest(const int *array, size_t size, int value)
{
    size_t best_index = 0;
    size_t i;
    int best_difference = abs(array[0] - value);

    for (i = 1; i < size; ++i) {
        int difference = abs(array[i] - value);

        if (difference < best_difference) {
            best_difference = difference;
            best_index = i;
        }
    }

    return best_index;
}

static void fill_array(int *array, size_t size,
                       int min_value, int max_value)
{
    size_t i;

    for (i = 0; i < size; ++i) {
        array[i] = random_int(min_value, max_value);
    }
}

static Status task_two(void)
{
    int *a = NULL;
    int *b = NULL;
    int *c = NULL;
    size_t size_a;
    size_t size_b;
    size_t i;
    Status status = STATUS_OK;

    size_a = (size_t)random_int(MIN_SIZE, MAX_SIZE);
    size_b = (size_t)random_int(MIN_SIZE, MAX_SIZE);

    a = malloc(size_a * sizeof(*a));
    b = malloc(size_b * sizeof(*b));
    c = malloc(size_a * sizeof(*c));

    if (a == NULL || b == NULL || c == NULL) {
        status = STATUS_MEMORY;
        goto cleanup;
    }

    fill_array(a, size_a, MIN_VALUE, MAX_VALUE);
    fill_array(b, size_b, MIN_VALUE, MAX_VALUE);

    for (i = 0; i < size_a; ++i) {
        size_t closest_index = find_closest(b, size_b, a[i]);

        c[i] = a[i] + b[closest_index];
    }

    printf("Задача 2\n");
    printf("Размер A: %zu\n", size_a);
    printf("Размер B: %zu\n", size_b);

    print_array("Первые элементы A", a,
                size_a < ARRAY_SIZE ? size_a : ARRAY_SIZE);
    print_array("Первые элементы B", b,
                size_b < ARRAY_SIZE ? size_b : ARRAY_SIZE);
    print_array("Первые элементы C", c,
                size_a < ARRAY_SIZE ? size_a : ARRAY_SIZE);

cleanup:
    free(a);
    free(b);
    free(c);

    return status;
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
        case STATUS_MEMORY:
            fprintf(stderr,
                    "Ошибка: не удалось выделить динамическую память.\n");
            break;
        case STATUS_RANGE:
            fprintf(stderr,
                    "Ошибка: нижняя граница больше верхней.\n");
            break;
    }
}

int main(int argc, char *argv[])
{
    int task;
    int a;
    int b;
    Status status;

    srand((unsigned int)time(NULL));

    if (argc < 2 || parse_int(argv[1], &task) != STATUS_OK) {
        status = STATUS_ARGUMENTS;
    } else if (task == 1) {
        if (argc != 4 ||
            parse_int(argv[2], &a) != STATUS_OK ||
            parse_int(argv[3], &b) != STATUS_OK) {
            status = STATUS_ARGUMENTS;
        } else {
            status = task_one(a, b);
        }
    } else if (task == 2) {
        if (argc != 2) {
            status = STATUS_ARGUMENTS;
        } else {
            status = task_two();
        }
    } else {
        status = STATUS_ARGUMENTS;
    }

    if (status != STATUS_OK) {
        print_error(status);
        return 1;
    }

    return 0;
}