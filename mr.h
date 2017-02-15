/******************************************************************************
 *
 * Functions which all memory reclamation schemes must provide.
 *
 *****************************************************************************/

#ifndef MR_H
#define MR_H

#include "node.h"

void mr_init();
void mr_thread_exit();
void mr_reinitialize();

#endif
