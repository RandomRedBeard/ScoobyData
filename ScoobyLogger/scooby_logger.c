#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// NCURSES Mac hack around for accessing _win_st attrs
#define NCURSES_OPAQUE 0

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

int handle_event(WINDOW* win, int cury, time_t start_timer, key_event* kev, FILE* fp) {
   // Get max y for accuracy
   int maxy = getmaxy(win);

   cury++;	
   if (cury >= maxy) {
      wmove(win, 0,0);
      wdeleteln(win);
      
      // In the case of a resize
      // cury may be beyond window
      cury = maxy - 1;
      wmove(win, cury,0);
   }

   int time_stamp = time(0) - start_timer;
   mvwprintw(win, cury, 0, "%s - %d", kev->event, time_stamp);
   write_key_event(fp, kev, time_stamp);

   return cury;
}

WINDOW* draw_event_win(WINDOW* win, event_list* ev) {

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

WINDOW* draw_main_win(WINDOW* win) {

   int y, x;
   getmaxyx(stdscr, y, x);

   int nlines = y * (EVENT_LIST_WINDOW_PERCENT - 1) / EVENT_LIST_WINDOW_PERCENT;
   int ncols = x;

   int ystart = y / EVENT_LIST_WINDOW_PERCENT;

   if (!win) {
      win = newwin(nlines, ncols, ystart, 0);
   } else {
      // Change upper limit on resize
      win->_begy = ystart;
      wresize(win, nlines, ncols);
   }

   return win;
   
}

int main(int argc, char** argv) {

   int inp;

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
   noecho();

   // Draw both windows on terminal
   WINDOW* event_win = draw_event_win(NULL, ev);
   WINDOW* main_win = draw_main_win(NULL);

   // curses display for windows
   wrefresh(event_win);
   wrefresh(main_win);

   wprintw(main_win, "Starting movie\n");

   int cury = 0;

   int pause_start = 0;
   int pause_time = 0;
   int paused = FALSE;

   key_event* pause_key_event = create_key_event(PAUSE_MOVIE, PAUSE_EVENT);
   key_event* unpause_key_event = create_key_event(PAUSE_MOVIE, UNPAUSE_EVENT);

   time_t start_timer = time(0);

   while((inp = wgetch(main_win)) != END_MOVIE) {

      if (inp == KEY_RESIZE) {
         event_win = draw_event_win(event_win, ev);
         main_win = draw_main_win(main_win);
         wrefresh(event_win);
         wrefresh(main_win);

         continue;
      }

      if (inp == PAUSE_MOVIE && !paused) {
         paused = TRUE;
         pause_start = time(0);

         cury = handle_event(main_win, cury, start_timer + pause_time, pause_key_event, fp);
      } else if (paused) {
         paused = FALSE;
         pause_time += time(0) - pause_start;

         cury = handle_event(main_win, cury, start_timer + pause_time, unpause_key_event, fp);
      }

      key_event* kev = get_key_event(ev, inp);
      if (!kev) {
         continue;
      }

      cury = handle_event(main_win, cury, start_timer + pause_time, kev, fp);
   }

   fclose(fp);

   destroy_key_event(pause_key_event);
   destroy_key_event(unpause_key_event);

   destroy_event_list(ev);

   endwin();
   return 0;
}
