#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>

#include "event_list.h"

char START_MOVIE;
char END_MOVIE;

char* tgets(char* buffer, int len) {
   fgets(buffer,len,stdin);
   int s_len = strlen(buffer);
   *(buffer + s_len - 1) = '\0';
   return buffer;
}

int readln(FILE* fp, char* buffer, int len) {
   int i = 0;
   int rval = 0;

   while(i < len && (*(buffer + i) = fgetc(fp)) != EOF && *(buffer + i) != '\n' && *(buffer + i) != '\r') {
      i++;
   }

   if (*(buffer + i) == EOF && i == 0) {
      rval = EOF;
   } else {
      rval = i;
   }

   *(buffer + i) = '\0';

   return rval;
}

event_list* read_events_from_file(const char* filepath, bool has_header) {
   FILE* fp = fopen(filepath,"r");

   if (!fp) {
      return NULL;
   }

   char buffer[128];

   event_list* ev = create_event_list();

   int i = 0;

   while(readln(fp, buffer, 128) != EOF) {

      if (strlen(buffer) == 0) {
         i++;
         continue;
      }

      if (i == 0 && has_header) {
         i++;
         continue;
      }

      char* after = strchr(buffer,',');
      *(buffer + (after - buffer)) = '\0';

      if (strlen(buffer) == 0 || strlen(after + 1) == 0) {
          i++;
          continue;
      }

      key_event* kev = create_key_event(*buffer, after + 1);
      add_key_event(ev,kev);

      i++;
   }

   fclose(fp);

   return ev;
}

int write_key_event(FILE* fp, key_event* kev, int time_stamp) {
   return fprintf(fp, "%s,%d\n", kev->event, time_stamp);
}

int main(int argc, char** argv) {

   char inp;

   event_list* ev = read_events_from_file("scooby-keys-config.csv", TRUE);

   if (!ev) {
      perror("Failed to read event list");
      return -1;
   }

   print_event_list(ev);

   printf("Movie Title: ");

   char movie[128];
   tgets(movie,127);

   char buffer[128];

   printf("Start Movie Key: ");
   START_MOVIE = *tgets(buffer, 127);
   printf("End Movie Key: " );
   END_MOVIE = *tgets(buffer, 127);

   printf("Waiting to start movie: ");

   while((inp = getchar()) != START_MOVIE) {
   }

   /*
    * Open new file with movie name + .csv
    */
   strcat(movie,".csv");
   FILE* fp = fopen(movie,"w");

   initscr();

   // Get window height
   int y;
   y = getmaxy(stdscr);

   printw("Starting movie\n");

   int cury = 0;

   time_t start_timer = time(0);

   noecho();

   while((inp = getch()) != END_MOVIE) {
      key_event* kev = get_key_event(ev, inp);
      if (!kev) {
         continue;
      }

      cury++;	
      if (cury == y) {
         move(0,0);
         deleteln();
         cury--;
         move(cury,0);
      }

      int time_stamp = time(0) - start_timer;
      mvprintw(cury, 0, "%s - %d", kev->event, time_stamp);
      write_key_event(fp, kev, time_stamp);
   }

   fclose(fp);

   destroy_event_list(ev);

   endwin();
   return 0;
}
