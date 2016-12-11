/*
   This file is part of QuasselC.

   QuasselC is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3.0 of the License, or (at your option) any later version.

   QuasselC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with QuasselC.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "quasselc.h"
#include "export.h"

/*
This file is here to declare symbols that apps should define.
Those are just weak stubs.
 */

void __attribute__((weak)) handle_backlog(struct message m, void *arg) {
	(void) arg;
	(void) m;
	fprintf(stderr, "You REALLY should define %s function", __FUNCTION__);
}

void __attribute__((weak)) handle_message(struct message m, void *arg) {
	(void) arg;
	(void) m;
	fprintf(stderr, "You REALLY should define %s function", __FUNCTION__);
}

void __attribute__((weak)) handle_sync(void* arg, object_t o, function_t f, ...) {
	(void) arg;
	//Should be used to ensure f consistency
	(void) o;
	(void) f;
	fprintf(stderr, "You REALLY should define %s function", __FUNCTION__);
}

void __attribute__((weak)) handle_event(void* arg, GIOChannel* h, event_t t, ...) {
	(void) arg;
	(void) h;
	(void) t;
	fprintf(stderr, "You REALLY should define %s function", __FUNCTION__);
}
