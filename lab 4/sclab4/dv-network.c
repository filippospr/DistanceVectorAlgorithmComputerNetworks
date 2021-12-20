/**
 * Implementing distributed, asynchronous, distance vector routing.
 *
 * Original source code by [anonymized].
 * Modified version by N. Ntarmos <ntarmos@cs.uoi.gr>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "node.h"

Node nodes[NUMNODES];
int TRACE = 1;
struct event;
void init();
void insertevent(struct event *);
void printevlist();
float better_rand();
unsigned getseed();

void creatertpkt(RtPkt *initrtpkt, int srcid, int destid, int* mincosts) {
	int i;
	initrtpkt->sourceid = srcid;
	initrtpkt->destid = destid;
	for (i = 0; i < 5; i++)
		initrtpkt->mincost[i] = mincosts[i];
}

static void err(char *str, int ret) {
	fprintf(stderr, "%s\n", str);
	exit(ret);
}

/*********************************************************************
 ****************** NETWORK EMULATION CODE STARTS BELOW **************
The code below emulates the layer 2 and below network environment:
  - emulates the transmission and delivery (with no loss and no
    corruption) between two physically connected nodes
  - calls the initializations routines rtinit0, etc., once before
    beginning emulation

THERE IS NO REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND THE
CODE BELOW.  YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY OF THE DATA
STRUCTURES BELOW.  If you're interested in how I designed the emulator, you're
welcome to look at the code - but again, you should have to, and you
definitely should not have to modify
**********************************************************************/

struct event {
	float evtime;		/* event time */
	int evtype;		/* event type code */
	int eventity;		/* entity where event occurs */
	RtPkt *rtpktptr;	/* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;	/* the event list */

/* possible events: */
#define  FROM_LAYER2     2
#define  LINK_CHANGE     10

float clocktime = 0.000;

/*********************************************************************
THIS IS THE MAIN ROUTINE.  IT SHOULD NOT BE TOUCHED AT ALL BY STUDENTS!
**********************************************************************/

int main(int argc, char **argv) {
	int i, opt;
	struct event *eventptr;

	while ((opt = getopt(argc, argv, "t:h")) != -1) {
		switch (opt) {
			case 't': 
				TRACE = atoi(optarg);
				break;
			case 'h':
			default:
				fprintf(stderr, "Usage: %s [-t trace_level]\n", argv[0]);
				return 1;
		}
	}


	setbuf(stdout, NULL);
	init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (!eventptr)
			break;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist)
			evlist->prev = NULL;
		if (TRACE > 1) {
			printf("MAIN: rcv event, t=%.3f, at %d",
					eventptr->evtime, eventptr->eventity);
			if (eventptr->evtype == FROM_LAYER2) {
				printf(" src:%2d, dest:%2d, contents: %3d %3d %3d %3d %3d\n",
						eventptr->rtpktptr->sourceid,
						eventptr->rtpktptr->destid,
						eventptr->rtpktptr->mincost[0],
						eventptr->rtpktptr->mincost[1],
						eventptr->rtpktptr->mincost[2],
						eventptr->rtpktptr->mincost[3],
						eventptr->rtpktptr->mincost[4]);
			}
		}
		clocktime = eventptr->evtime;    /* update time to next event time */
		if (eventptr->evtype == FROM_LAYER2) {
			if (eventptr->eventity >= 0 && eventptr->eventity < NUMNODES)
				updateRT(&nodes[eventptr->eventity], eventptr->rtpktptr);
			else
				err("Panic: unknown event entity", 1);
		} else
			err("Panic: unknown event type", 1);
		if (eventptr->evtype == FROM_LAYER2)
			free(eventptr->rtpktptr);        /* free memory for packet, if any */
		free(eventptr);                    /* free memory for event struct   */
	}

	for (i = 0; i < NUMNODES; i++)
		printRT(&nodes[i]);
	printf("\nSimulator terminated at t=%f, no packets in medium\n", clocktime);
	return 0;
}

void init() {                       /* initialize the simulator */
	int i;
	float sum, avg;

	initcosts();
	srandom(getseed());       /* init random number generator */
	sum = 0.0;                /* test random number generator for students */
	for (i = 0; i < 1000; i++)
		sum += better_rand();    /* better_rand() should be uniform in [0,1] */
	avg = sum / 1000.0;
	if (avg < 0.25 || avg > 0.75)
		err("It is likely that random number generation on your machine is\ndifferent from what this emulator expects. Please take a look\nat the routine better_rand() in the emulator code. Sorry...", 1);

	clocktime = 0.0;                /* initialize time to 0.0 */
	for (i = 0; i < NUMNODES; i++) {
		nodes[i].id = i;
		initRT(&nodes[i]);
	}
}

/**
 * Returns a quasi-random unsigned int, to be used as the PRNG seed.
 */
unsigned getseed(void) {
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return (unsigned)tv.tv_usec;
}

/**
 * A "better" implementation of rand, using random(3). random(3) returns a
 * long, but RAND_MAX on SunOS (and possibly other older Unices) is defined as
 * an int so it can't be used to scale the output. Since the maximum value of
 * random(3) is defined to be 2^31 - 1, this limit is hardcoded in the code
 * below.
 */
float better_rand(void)
{ 

return random() / (float)((1UL << 31) - 1UL); 

}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/


void insertevent(struct event *p) {
	struct event *q,*qold;

	if (TRACE > 3) {
		printf("            INSERTEVENT: time is %lf\n",clocktime);
		printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (!q) {   /* list is empty */
		evlist = p;
		p->next = p->prev = NULL;
	} else {
		for (qold = q; q !=NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
		if (!q) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		} else if (q == evlist) { /* front of list */
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = evlist = p;
		} else {     /* middle of list */
			p->next = q;
			p->prev = q->prev;
			q->prev = q->prev->next = p;
		}
	}
}

void printevlist() {
	struct event *q;
	printf("--------------\nEvent List Follows:\n");
	for(q = evlist; q; q = q->next) {
		printf("Event time: %f, type: %d entity: %d\n",
				q->evtime, q->evtype, q->eventity);
	}
	printf("--------------\n");
}


/************************** TOLAYER2 ***************/
void tolayer2(RtPkt packet) {
	RtPkt *mypktptr;
	struct event *evptr, *q;
	float lastime;
	int i;

	/* be nice: check if source and destination id's are reasonable */
	if (packet.sourceid < 0 || packet.sourceid > NUMNODES - 1) {
		fprintf(stderr, "WARNING: illegal source id in your packet, ignoring packet!\n");
		return;
	}
	if (packet.destid < 0 || packet.destid > NUMNODES - 1) {
		fprintf(stderr, "WARNING: illegal dest id in your packet, ignoring packet!\n");
		return;
	}
	if (packet.sourceid == packet.destid)  {
		fprintf(stderr, "WARNING: source and destination id's the same, ignoring packet!\n");
		return;
	}
	if (connectcosts[packet.sourceid][packet.destid] == 999)  {
		fprintf(stderr, "WARNING: source and destination not connected, ignoring packet!\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (RtPkt *)malloc(sizeof(RtPkt));
	mypktptr->sourceid = packet.sourceid;
	mypktptr->destid = packet.destid;
	memcpy(mypktptr->mincost, packet.mincost, NUMNODES * sizeof(packet.mincost[0]));
	if (TRACE > 2)  {
		printf("    TOLAYER2: source: %d, dest: %d\n              costs:",
				mypktptr->sourceid, mypktptr->destid);
		for (i = 0; i < NUMNODES; i++)
			printf("%d  ",mypktptr->mincost[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype =  FROM_LAYER2;   /* packet will pop out from layer3 */
	evptr->eventity = packet.destid; /* event occurs at other entity */
	evptr->rtpktptr = mypktptr;       /* save ptr to my copy of packet */

	/* finally, compute the arrival time of packet at the other end.
	   medium can not reorder, so make sure packet arrives between 1 and 10
	   time units after the latest arrival time of packets
	   currently in the medium on their way to the destination */
	lastime = clocktime;
	for (q = evlist; q ; q = q->next)
		if ((q->evtype == FROM_LAYER2  && q->eventity == evptr->eventity))
			lastime = q->evtime;
	evptr->evtime = lastime + 2.*better_rand();

	if (TRACE > 2)
		printf("    TOLAYER2: scheduling arrival on other side\n");
	insertevent(evptr);
}

