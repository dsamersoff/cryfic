/*
 * Copyright © 2026 Dmitry Samersoff, all rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: dsForm.cxx,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 *
 */

#include "dsForm.hpp"

using namespace std;

ostream& dsForm::form(ostream& os, const char *format,    ... )
{
  va_list ap;
  va_start(ap, format);
  ostream& ros = dsForm::vform(os,format,ap);
  va_end(ap);

  return ros;
}

ostream& dsForm::vform(ostream& os, const  char *format, va_list ap )
{
  const char* fptr = format;

  while (1)
  {
    if (*fptr == '%')
    {
      ++fptr;

      /* init state */
      int width     = -1;
      int modifier  = 'i';
      int fill      = ' ';
      int left      = 0;

      os.unsetf( ios::uppercase );

      if (*fptr == '-') {
        left = 1;
        ++fptr;
      }
      else {
        left = 0;
      }

      // set zerro fill if necessary
      if (*fptr == '0') {
        fill = '0';
        ++fptr;
      }

      //XXX ??
      if (*fptr == '.') {
        ++fptr;
        fill = '0';
      }

      // extract width from varg or from format
      if (*fptr == '*') {
        width = va_arg(ap, int);
        ++ fptr;
      }
      else {
        for (; (*fptr >= '0' && *fptr <= '9'); ++fptr) {
          if (width < 0) {
            width = 0;
          }
          width = width*10 + (*fptr - '0');
        }
      }

      if (*fptr == 'l') { // handle %lX && %ld
        modifier = 'l';
        ++fptr;
      }

      switch (*fptr) {
        case 'c': {
          int c = va_arg(ap, int);
          os << (char) c;
        }
        break;
        case 's': {
          char* s = va_arg(ap, char *);
          if (!s) {
             s = (char *) "(null)";
          }
          if(!left){
            os << setw(width);
          }
          else {
            // Manually emulate left
            int len = strlen(s);
            if (width > 0 && len < width) {
              std::cout << std::string(width - len, ' ');
            }
          }
          os << s;
        }
        break;
        case 'd' : {
            os << setw(width);
            os << setfill( (char) fill);
            os << dec;
            if (modifier == 'l') {
              long ld = va_arg(ap, long);
              os << ld;
            }
            else if (modifier == 'i') {
              int id = va_arg(ap, int);
              os << id;
            }
          }
          break;
        case 'X' :
          os.setf(ios::uppercase);
        case 'p' :
        case 'x' : {
          os << setw(width);
          os << setfill( (char) fill);
          os << hex;
          if (modifier == 'l') {
            long ld = va_arg(ap, long);
            os << ld;
          }
          else if (modifier == 'i') {
            int id = va_arg(ap, int);
            os << id;
          }
        }
        break;
        case 'f' : {
            os << setiosflags(ios::fixed);
            os << setprecision( (width >= 0) ? width : 6 );
            double ld = va_arg(ap, double);
            os << ld;
        }
        break;
        case 'E': {
            os << strerror(errno) << "(" << errno << ")";
        }
        break;
      }

      ++fptr;   // don't copy format characters
    } // if %

    if (!*fptr)
      break;

    os << (char) *fptr;
    ++fptr;
  } // while

  return os;
}