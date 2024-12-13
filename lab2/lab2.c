#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void first_task(){
    int a=322;
    int b=1487;
    int *p_a = &a;
    int *p_b = &b;
    if (*p_a > *p_b) {
        *p_a *= 2;
    } else {
        *p_b /= 2;
    }
    printf("Changed value a: %d\n", *p_a);
    printf("Changed value b: %d\n", *p_b);
}

void second_task(){
    float *ptr1, *ptr2;
    ptr1 = (float *)malloc(sizeof(float));
    ptr2 = (float *)malloc(sizeof(float));
    if (ptr1 == NULL || ptr2 == NULL) {
        printf("Malloc error\n");
        return;
    }
    printf("Enter value of a:\n");
    scanf("%f", ptr1);
    printf("Enter value of b:\n");
    scanf("%f", ptr2);
    *ptr1 *= 2;
    printf("Changed value a: %.2f\n", *ptr1);
    printf("Value b: %.2f\n", *ptr2);
    free(ptr1);
    free(ptr2);
}

void third_task(){
    int n, m;
    int *x, *y;
    int count = 0;
    printf("Enter size of first array (n): ");
    scanf("%d", &n);
    printf("Enter size of second array (m): ");
    scanf("%d", &m);
    x = (int *)malloc(n * sizeof(int));
    y = (int *)malloc(m * sizeof(int));
    if (x == NULL || y == NULL) {
        printf("Malloc error\n");
        return;
    }
    printf("Enter elements of first array (x):\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &x[i]);
    }
    printf("Enter elements of second array (y):\n");
    for (int i = 0; i < m; i++) {
        scanf("%d", &y[i]);
    }
    if (n < 2) {
        printf("Two elements need to be in the first array\n");
        free(x);
        free(y);
        return;
    }
    int second_element = x[1];
    for (int i = 0; i < m; i++) {
        if (y[i] == second_element)
            count++;
    }
    printf("The second element of the first array occurs %d times in the second array.\n", count);
    free(x);
    free(y);
}

void selection_sort(int *arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < size; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        int temp = arr[i];
        arr[i] = arr[min_idx];
        arr[min_idx] = temp;
    }
}

void four_task(){
    int n, count = 0;
    printf("Enter size of the array: ");
    scanf("%d", &n);
    int *a = (int *)malloc(n * sizeof(int));
    if (a == NULL) {
        printf("Malloc error\n");
        return;
    }
    printf("Enter elements of the array:\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &a[i]);
    }
    int *b = (int *)malloc(n * sizeof(int));
    if (b == NULL) {
        printf("Malloc error\n");
        free(a);
        return;
    }
    for (int i = 0; i < n; i++) {
        if (a[i] > 0) {
            b[count] = a[i] * 3;
            count++;
        }
    }
    printf("Array b: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", b[i]);
    }
    printf("\n");
    selection_sort(b, count);
    printf("Sorted array b: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", b[i]);
    }
    printf("\n");
    free(a);
    free(b);
}
void minutes_to_seconds(float *time) {
    *time= (*time * (float)60.0)/1;
}

void fifth_task(){
    float time=(float)33.2;
    printf("%.2f min = ", time);
    minutes_to_seconds(&time);
    printf("%.2f secs.\n", time);
    time=(float)43.2;
    printf("%.2f min = ", time);
    minutes_to_seconds(&time);
    printf("%.2f secs.\n", time);
}

#define MAX_RESIDENTS 100
#define MAX_STRING_LEN 100

struct Address {
    char * street;
    int house;
    int apartment;
};

typedef struct Resident {
    char * surname;
    char * city;
    struct Address address;
}resident;

int compare_addresses(struct Address *address1, struct Address *address2) {
    if (strcmp(address1->street, address2->street) != 0) {
        return 0;
    }
    if (address1->house != address2->house) {
        return 0;
    }
    if (address1->apartment != address2->apartment) {
        return 0;
    }

    return 1;
}

void find_residents_with_same_address(struct Resident residents[], int num_residents) {
    for (int i = 0; i < num_residents; i++) {
        for (int j = i + 1; j < num_residents; j++) {
            if (compare_addresses(&residents[i].address, &residents[j].address)) {
                printf("Residents with same address:\n");
                printf("1. %s, Town: %s, Address: %s, House: %d, Apartment: %d\n",
                       residents[i].surname, residents[i].city,
                       residents[i].address.street, residents[i].address.house, residents[i].address.apartment);
                printf("2. %s, Town: %s, Address: %s, House: %d, Apartment: %d\n",
                       residents[j].surname, residents[j].city,
                       residents[j].address.street, residents[j].address.house, residents[j].address.apartment);
                return;
            }
        }
    }
    printf("No residents with same addressed found.\n");
}

void six_task(){
    int number_of_residents =100;
    srand(time(NULL));
    resident * residents = (resident *) malloc(sizeof(resident)*number_of_residents);
    char * towns[5] = {"Moscow", "New York", "Las Vegas", "Kherson", "Saint Petersburg"};
    char * surnames[5] = {"Kolesov", "Pavlov", "Volkov", "Yakubiv", "Mishenko"};
    char * streets[5] = {"Amurskaya", "Goida", "Vedenyapina", "Lenina", "Proletarsky Prospect"};
    for (int i = 0; i < number_of_residents; ++i) {
        residents[i].address.street=streets[rand()%5];
        residents[i].address.apartment=rand()%5;
        residents[i].address.house=rand()%5;
        residents[i].surname = surnames[rand()%5];
        residents[i].city = towns[rand()%5];
    }
    printf("Residents:\n");
    for (int i = 0; i < number_of_residents; ++i) {
        printf("%d %s %s %s %d %d\n",i+1,residents[i].surname,residents[i].city, residents[i].address.street,residents[i].address.house, residents[i].address.apartment);
    }
    printf("\n");
    find_residents_with_same_address(residents,number_of_residents);
}

#define MAX_FLIGHTS 10
#define MAX_DEST_LEN 100

struct Flight {
    char destination[MAX_DEST_LEN];
    char departure_time[MAX_DEST_LEN];
    char arrival_time[MAX_DEST_LEN];
    int flight_time;
    float ticket_price;
};

int contains_two_a(const char* destination) {
    int count = 0;
    for (int i = 0; destination[i] != '\0'; i++) {
        if (tolower(destination[i]) == 'a') {
            count++;
        }
    }
    return count == 2;
}

int filter_flights(const struct Flight* flights, struct Flight* filtered_flights) {
    int filtered_count = 0;
    for (int i = 0; i < MAX_FLIGHTS; i++) {
        if (contains_two_a(flights[i].destination)) {
            filtered_flights[filtered_count++] = flights[i];
        }
    }
    return filtered_count;
}

void sort_by_destination(struct Flight* flights, int n) {
    struct Flight temp;
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (strcmp(flights[i].destination, flights[j].destination) > 0) {
                temp = flights[i];
                flights[i] = flights[j];
                flights[j] = temp;
            }
        }
    }
}
void initialize_flights(struct Flight* flights) {
    struct Flight data[MAX_FLIGHTS] = {
            {"Paris", "10:00", "12:00", 120, 5000.0},
            {"Berlin", "08:30", "11:00", 150, 7000.0},
            {"Barcelona", "14:00", "16:00", 120, 6500.0},
            {"Amsterdam", "16:00", "18:30", 150, 7500.0},
            {"London", "09:00", "11:30", 150, 6000.0},
            {"Madrid", "07:30", "09:30", 120, 5500.0},
            {"Athens", "13:00", "15:30", 150, 6800.0},
            {"New York", "11:00", "14:00", 180, 9000.0},
            {"Tokyo", "20:00", "22:30", 150, 12000.0},
            {"Los Angeles", "18:00", "21:00", 180, 10500.0}
    };

    for (int i = 0; i < MAX_FLIGHTS; i++) {
        flights[i] = data[i];
    }
}

void print_flight(const struct Flight* flight) {
    printf("Destination: %s\n", flight->destination);
    printf("Departure_time: %s\n", flight->departure_time);
    printf("Arrival_time: %s\n", flight->arrival_time);
    printf("Flight_time: %d min\n", flight->flight_time);
    printf("Ticket_price: %.2f rub\n\n", flight->ticket_price);
}

void print_all_flights(const struct Flight* flights, int n) {
    for (int i = 0; i < n; i++) {
        print_flight(&flights[i]);
    }
}

void seven_task(){
    struct Flight flights[MAX_FLIGHTS];
    struct Flight filtered_flights[MAX_FLIGHTS];
    initialize_flights(flights);
    int filtered_count = filter_flights(flights, filtered_flights);
    sort_by_destination(filtered_flights, filtered_count);
    printf("Flights with two 'a' in destination:\n");
    print_all_flights(filtered_flights, filtered_count);
}

int main() {
    seven_task();
    return 0;
}