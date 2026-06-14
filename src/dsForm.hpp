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
 *  $Id: dsForm.h,v 1.1.1.1 2004/09/10 08:12:19 dsamersoff Exp $
 */

#ifndef dsForm_h
#define dsForm_h

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <iostream>
#include <iomanip>

class dsForm
{
  public:
    // Printf like functionality on top of iostream
    static std::ostream& form(std::ostream& os, const char *format,   ... );
    static std::ostream& vform(std::ostream& os, const char *format, va_list ap );
};

#endif
