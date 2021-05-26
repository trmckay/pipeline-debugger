// 
// Writen by Hugh Smith, April 2020
//
// Provides an interface to the poll() library.  Allows for
// adding a file descriptor to the set, removing one and calling poll.
// Feel free to copy, just leave my name in it, use at your own risk.
//


#ifndef __POLLLIB_H__
#define __POLLLIB_H__

#define POLL_SET_SIZE 10
#define POLL_WAIT_FOREVER -1

void setupPollSet();
void addToPollSet(int socketNumber);
void removeFromPollSet(int socketNumber);
int pollCall(int timeInMilliSeconds);

#endif