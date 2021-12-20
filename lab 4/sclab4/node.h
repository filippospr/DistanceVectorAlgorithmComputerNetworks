#ifndef __NODE_H__
#define __NODE_H__

#define NUMNODES (5)

/* a RtPkt is the packet sent from one routing update process to
   another via the call tolayer3() */
typedef struct {
	int sourceid;          /* id of sending router sending this pkt */
	int destid;            /* id of router to which pkt being sent
	                          (must be an immediate neighbor) */
	int mincost[NUMNODES]; /* min cost to node 0 ... 3 */
} RtPkt;

typedef struct {
	int id;
	struct {
		int cost[NUMNODES];
		int nexthop[NUMNODES];
	} rt;
} Node;

extern int connectcosts[NUMNODES][NUMNODES];
extern void tolayer2(RtPkt);

/* Functions to be implemented */
extern void initcosts();
extern void updateRT(Node*, RtPkt*);
extern void initRT(Node*);
extern void printRT(Node*);

#endif
