#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "BENSCHILLIBOWL.h"

#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS NUM_CUSTOMERS * ORDERS_PER_CUSTOMER

/* Global variable for the restaurant. */
BENSCHILLIBOWL *bcb;

/**
 * Thread function that represents a customer.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;

    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
        Order* order = malloc(sizeof(Order));
        if (order == NULL) {
            perror("malloc");
            return NULL;
        }

        order->menu_item = PickRandomMenuItem();
        order->customer_id = customer_id;
        order->next = NULL;

        int order_number = AddOrder(bcb, order);

        printf("Customer #%d ordered %s (order #%d)\n",
               customer_id, order->menu_item, order_number);
    }

    return NULL;
}

/**
 * Thread function that represents a cook.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
    int orders_fulfilled = 0;

    while (1) {
        Order* order = GetOrder(bcb);

        if (order == NULL) {
            break;
        }

        printf("Cook #%d fulfilled order #%d for customer #%d: %s\n",
               cook_id, order->order_number, order->customer_id,
               order->menu_item);

        free(order);
        orders_fulfilled++;
    }

    printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
    return NULL;
}

/**
 * Runs when the program begins executing.
 */
int main() {
    srand(time(NULL));

    bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);
    if (bcb == NULL) {
        fprintf(stderr, "Failed to open restaurant\n");
        return 1;
    }

    pthread_t customers[NUM_CUSTOMERS];
    pthread_t cooks[NUM_COOKS];

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer,
                       (void*)(long)(i + 1));
    }

    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_create(&cooks[i], NULL, BENSCHILLIBOWLCook,
                       (void*)(long)(i + 1));
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pthread_join(customers[i], NULL);
    }

    for (int i = 0; i < NUM_COOKS; i++) {
        pthread_join(cooks[i], NULL);
    }

    CloseRestaurant(bcb);
    return 0;
}