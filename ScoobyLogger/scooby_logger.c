#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <math.h>

#include "event_list.h"

#define EVENT_LIST_WINDOW_PERCENT 4

char START_MOVIE;
char END_MOVIE;
char PAUSE_MOVIE;

const char* PAUSE_EVENT = "user pause";
const char* UNPAUSE_EVENT = "user unpause";

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

int handle_event(WINDOW* win, int cury, time_t start_timer, key_event* kev, FILE* fp, int maxy) {
      cury++;	
      if (cury == maxy) {
         wmove(win, 0,0);
         wdeleteln(win);
         cury--;
         wmove(win, cury,0);
      }

      int time_stamp = time(0) - start_timer;
      mvwprintw(win, cury, 0, "%s - %d", kev->event, time_stamp);
      write_key_event(fp, kev, time_stamp);

      return cury;
}

WINDOW* draw_event_list(WINDOW* win, event_list* ev) {

   int y, x;
   getmaxyx(stdscr, y, x);

   if (!win) {
      win = newwin(y / EVENT_LIST_WINDOW_PERCENT, x, 0, 0);
   } else {
      wclear(win);
      wrefresh(win);
      wresize(win, y / EVENT_LIST_WINDOW_PERCENT, x);
   }

   int win_height = getmaxy(win);
   int win_width = getmaxx(win);

   int count = (int)(ev->size / win_height);
   //Temp soln to prevent over stepping window height
   count++;

   int width = win_width / count;

   key_event* iter = ev->head;

   int column = 0;
   int row = 0;

   while(iter) {
      mvwprintw(win, row, column * width, "%c - %s", iter->key, iter->event);

      column++;

      if (column == count) {
         column = 0;
         row++;
      }

      iter = iter->next;
   }
   return win;
}

WINDOW* draw_log_window(struct _win_st* win) {

   int y, x;
   getmaxyx(stdscr, y, x);

   int nlines = y * (EVENT_LIST_WINDOW_PERCENT - 1) / EVENT_LIST_WINDOW_PERCENT;
   int ncols = x;

   int ystart = y / EVENT_LIST_WINDOW_PERCENT;

   if (!win) {
      win = newwin(nlines, ncols, ystart, 0);
   } else {
      win->_begy = ystart;
      wresize(win, nlines, ncols);
   }

   return win;
   
}

int main() {
   event_list* ev = read_events_from_file("scooby-keys-config.csv", TRUE);

   initscr();
   noecho();

   int y, x;
   getmaxyx(stdscr, y, x);
   
   WINDOW* win = draw_event_list(NULL, ev);
   
   WINDOW* bot = draw_log_window(NULL);

   wrefresh(win);
   wrefresh(bot);

   int inp;
   int cury = 0;

   while((inp = wgetch(bot)) != 'q') {
      if (inp == KEY_RESIZE) {
         win = draw_event_list(win, ev);
         bot = draw_log_window(bot);
         wrefresh(win);
         wrefresh(bot);
      } else {
         int maxy = getmaxy(bot);
         cury++;	
         if (cury >= maxy) {
            wmove(bot, 0,0);
            wdeleteln(bot);
            cury = maxy - 1;
            wmove(bot, cury,0);
         }
         wprintw(bot, "Bot\n");
         wrefresh(bot);
      }
   }

   endwin();

   return 0;
}

int mainx(int argc, char** argv) {

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
   printf("Pause Movie Key: ");
   PAUSE_MOVIE = *tgets(buffer, 127);

   printf("Waiting to start movie: ");

   while((inp = getchar()) != START_MOVIE) {
   }

   /*
    * Open new file with movie name + .csv
    */
   strcat(movie,".csv");
   FILE* fp = fopen(movie,"w");

   initscr();

   WINDOW* win = stdscr;

   // Get window height
   int y;
   y = getmaxy(win);

   wprintw(win, "Starting movie\n");

   int cury = 0;

   int pause_start = 0;
   int pause_time = 0;
   int paused = FALSE;

   key_event* pause_key_event = create_key_event(PAUSE_MOVIE, PAUSE_EVENT);
   key_event* unpause_key_event = create_key_event(PAUSE_MOVIE, UNPAUSE_EVENT);

   time_t start_timer = time(0);

   noecho();

   while((inp = wgetch(win)) != END_MOVIE) {

      if (inp == PAUSE_MOVIE && !paused) {
         paused = TRUE;
         pause_start = time(0);

         cury = handle_event(win, cury, start_timer + pause_time, pause_key_event, fp, y);
      } else if (paused) {
         paused = FALSE;
         pause_time += time(0) - pause_start;

         cury = handle_event(win, cury, start_timer + pause_time, unpause_key_event, fp, y);
      }

      key_event* kev = get_key_event(ev, inp);
      if (!kev) {
         continue;
      }

      cury = handle_event(win, cury, start_timer + pause_time, kev, fp, y);
   }

   fclose(fp);

   destroy_key_event(pause_key_event);
   destroy_key_event(unpause_key_event);

   destroy_event_list(ev);

   endwin();
   return 0;
}
