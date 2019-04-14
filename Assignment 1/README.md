## Assignment 1
02/2019 — Parser library for iCalendar files.

### To Run

Running `make` or `make all` will produce two shared object libraries: liblist.so and libcal.so, both in the /bin directory.

***

See CalendarParser.h for the API. This first implementation supports createCalendar(), printCalendar(), deleteCalendar(), printError(), and the helper functions for the linked list. Other helper functions that are not externally exposed are also in CalendarParser.c, but use CalendarParserUtilities.h as a header file.
