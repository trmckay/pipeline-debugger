//
// Written Hugh Smith, Updated: April 2020
// Use at your own risk.  Feel free to copy, just leave my name in it.
//

#include <poll.h>
#include <stdlib.h>
#include <stdio.h>

#include "pollLib.h"


// Poll global variables 
static struct pollfd * pollFileDescriptors;
static int maxFileDescriptor = 0;
static int currentPollSetSize = 0;

static void growPollSet(int newSetSize);

// Poll functions (setup, add, remove, call)
void setupPollSet()
{
	currentPollSetSize = POLL_SET_SIZE;
	pollFileDescriptors = (struct pollfd *) calloc(POLL_SET_SIZE, sizeof(struct pollfd));
}


void addToPollSet(int socketNumber)
{
	
	if (socketNumber >= currentPollSetSize)
	{
		// needs to increase off of the biggest socket number since
		// the file desc. may grow with files open or sockets
		// so socketNumber could be much bigger than currentPollSetSize
		growPollSet(socketNumber + POLL_SET_SIZE);		
	}
	
	if (socketNumber + 1 >= maxFileDescriptor)
	{
		maxFileDescriptor = socketNumber + 1;
	}

	pollFileDescriptors[socketNumber].fd = socketNumber;
	pollFileDescriptors[socketNumber].events = POLLIN;
}

void removeFromPollSet(int socketNumber)
{
	pollFileDescriptors[socketNumber].fd = 0;
	pollFileDescriptors[socketNumber].events = 0;
}

int pollCall(int timeInMilliSeconds)
{
	// returns the socket number if one is ready for read
	// returns -1 if timeout occurred
	// if timeInMilliSeconds == -1 blocks forever (until a socket ready)
	// (this -1 is a feature of poll)
	
	int i = 0;
	int returnValue = -1;
	int pollValue = 0;
	
	if ((pollValue = poll(pollFileDescriptors, maxFileDescriptor, timeInMilliSeconds)) < 0)
	{
		perror("pollCall");
		exit(-1);
	}	
			
	// check to see if timeout occurred (poll returned 0)
	if (pollValue > 0)
	{
		// see which socket is ready
		for (i = 0; i < maxFileDescriptor; i++)
		{
			//if(pollFileDescriptors[i].revents & (POLLIN|POLLHUP|POLLNVAL)) 
			//Could just check for some revents, but want to catch any of them
			//Otherwise, this could mask an error (eat the error condition)
			if(pollFileDescriptors[i].revents > 0) 
			{
				//printf("for socket %d poll revents: %d\n", i, pollFileDescriptors[i].revents);
				returnValue = i;
				break;
			} 
		}

	}
	
	// Ready socket # or -1 if timeout/none
	return returnValue;
}
static void growPollSet(int newSetSize)
{
	int i = 0;
	
	// just check to see if someone screwed up
	if (newSetSize <= currentPollSetSize)
	{
		printf("Error - current poll set size: %d newSetSize is not greater: %d\n",
			currentPollSetSize, newSetSize);
		exit(-1);
	}
	
	printf("Increasing poll set from: %d to %d\n", currentPollSetSize, newSetSize);
	pollFileDescriptors = realloc(pollFileDescriptors, newSetSize * sizeof(struct pollfd));	
	
	// zero out the new poll set elements
	for (i = currentPollSetSize; i < newSetSize; i++)
	{
		pollFileDescriptors[i].fd = 0;
		pollFileDescriptors[i].events = 0;
	}
	
	currentPollSetSize = newSetSize;
}



