#include <stdlib.h>
#include <stdio.h>

struct key_event {
   struct key_event* next;
   char key;
   char* event;
};

typedef struct key_event key_event;

typedef struct {
   key_event* head;
   key_event* tail;
   unsigned int size;
} event_list;

key_event* create_key_event(char key, const char* event) {
   key_event* kev = (key_event*)malloc(sizeof(key_event));

   kev->key = key;
   kev->event = strdup(event);

   kev->next = (key_event*)NULL;

   return kev;
}

int destroy_key_event(key_event* kev) {
   if (!kev) {
      return -1;
   }

   if (kev->event) {
      free(kev->event);
   }

   free(kev);

   return 0;
}

event_list* create_event_list() {

   event_list* ev = (event_list*)malloc(sizeof(event_list));

   ev->head = ev->tail = (key_event*)NULL;

   ev->size = 0;

   return ev;
}

int add_key_event(event_list* ev, key_event* kev) {
   if (!ev->head && !ev->tail) {
      ev->head = ev->tail = kev;
   } else {
      ev->tail->next = kev;
      ev->tail = kev;
   }

   return ++ev->size;
}

/*
 * TODO: remove key event
 */

int destroy_event_list(event_list* ev) {
   while(ev->head) {
      key_event* kev = ev->head;
      ev->head = ev->head->next;
      kev->next = (key_event*)NULL;
      destroy_key_event(kev);
   }
   return 0;
}

void print_event_list(event_list* ev) {
   printf("Size %d\n", ev->size);
   key_event* iter = ev->head;
   while(iter) {
      printf("%c - %s\n", iter->key, iter->event);
      iter = iter->next;
   }
}

key_event* get_key_event(event_list* ev, char key) {
   key_event* iter = ev->head;
   while(iter) {
      if (key == iter->key) {
         return iter;
      }
      iter = iter->next;
   }

   return (key_event*) NULL;
}
