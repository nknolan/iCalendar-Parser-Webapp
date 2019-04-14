## iCalendar Parser / Webapp
Class project for Software Systems Development and Integration. The individual components are:

1. iCalendar parser. Written in C11, implements the iCalendar specification as laid out in [RFC 5545](https://tools.ietf.org/html/rfc5545).
2. Additions to the original iCalendar parser. The full parser can read, write, and validate iCalendar files, as well as convert information from .ics files into JSON strings.
3. Webapp for reading and manipulating iCalendar files. Frontend is Bootstrap/jQuery/AJAX, backend is Nodejs with FFI as a wrapper around the parser library.
4. Added an interface for uploading iCalendar data to a MariaDB server and organizing calendar information.
