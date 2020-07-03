#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc, free, srand, rand */

/*******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
	 are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
	 or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
	 (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char *datasent);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
int send_base_A;	// A send측의 window sendbase
int send_base_B;	// B send측의 window sendbase
int nextseqnum_A;	// A send측의 다음 보낼 packet위치
int nextseqnum_B;	// B send측의 다음 보낼 packet위치
int sequence_A;		// A sequence
int sequence_B;		// B sequence
int acknum_A;		// A acknum
int acknum_B;		// B acknum
struct pkt pkt_A[32];	//A output에서 보내는 pkt
struct pkt pkt_B[32];	//B output에서 보내는 pkt
int expectedseqnum_A;	// A receive측의 원하는 seqnum번호
int expectedseqnum_B;	// B receive측의 원하는 seqnum번호

/* Here I define some function prototypes to avoid warnings */
/* in my compiler. Also I declared these functions void and */
/* changed the implementation to match */
void init();
void generate_next_arrival();
void insertevent(struct event *p);

// checksum 구하는 function
int getCheckSum(struct pkt *packet)
{
	int checksum = 0;
	checksum += packet->seqnum;	//seqnum
	checksum += packet->acknum;	//acknum
	for (int i = 0; i < 20; i++)
		checksum += packet->payload[i];	//data
	return checksum;	//모두 더하여 checksum설정
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	pkt_A[sequence_A].seqnum = sequence_A;	//make packet
	memcpy(pkt_A[sequence_A].payload, message.data, 20);	//store data
	if (nextseqnum_A < send_base_A + 8)	//window에 여유가 있다면
	{
		if (nextseqnum_A == send_base_A) {	//둘이 같다면
			starttimer(0, (float)15);	//타이머 설정
		}
		pkt_A[nextseqnum_A].acknum = acknum_A;	//acknum설정
		pkt_A[nextseqnum_A].checksum = getCheckSum(&pkt_A[nextseqnum_A]);	//checksum
		printf("A_output -> B_input : [seq : %d] [ack : %d] [data : %s]\n", pkt_A[nextseqnum_A].seqnum, pkt_A[nextseqnum_A].acknum, message.data);
		tolayer3(0, pkt_A[nextseqnum_A]);	//전송
		nextseqnum_A++;	//next seqnum +1
	}
	sequence_A++;	//layer5로부터 받아올 data index +1
}

void B_output(struct msg message)  /* need be completed only for extra credit */
{
	pkt_B[sequence_B].seqnum = sequence_B;	//seqnum설정
	memcpy(pkt_B[sequence_B].payload, message.data, 15);	//data설정
	if (nextseqnum_B < send_base_B + 8)	//window에 여유가 있다면
	{
		if (nextseqnum_B == send_base_B)	//만약 둘이 같다면
			starttimer(1, (float)15);	//타이머를 설정한다
		pkt_B[nextseqnum_B].acknum = acknum_B;	//acknum설정 (piggyback)
		pkt_B[nextseqnum_B].checksum = getCheckSum(&pkt_B[nextseqnum_B]);	//checksum설정
		printf("B_output -> A_input : [seq : %d] [ack : %d] [data : %s]\n", pkt_B[nextseqnum_B].seqnum, pkt_B[nextseqnum_B].acknum, message.data);
		tolayer3(1, pkt_B[nextseqnum_B]);	//전송
		nextseqnum_B++;	//nextseqnum +1
	}
	sequence_B++;	//layer5로부터 받을 index +1
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	if (packet.checksum != getCheckSum(&packet))	//if pkt corrupted
	{
		printf("A_input packet corrupted : [seq : %d]\n", packet.seqnum);
	}
	else if (packet.seqnum != expectedseqnum_A)	//중복 패킷이거나 원하지않은 패킷이거나
	{
		printf("A_input not expected sequence number : [expected : %d] [seq : %d]\n", expectedseqnum_A, packet.seqnum);
	}
	else {	//패킷 제대로 수신
		printf("A_input receive packet : [seq : %d] [ack : %d] [data : %s]\n", packet.seqnum, packet.acknum, packet.payload);
		tolayer5(0, packet.payload);	//위로 전송
		send_base_A = packet.acknum + 1;	//sendbase재조정
		if (send_base_A == nextseqnum_A) {
			stoptimer(0);	//데이터가 끝났다면 타이머 멈춤
		}
		else
			starttimer(0, (float)15);	//아니면 타이머 재설정
		acknum_A = expectedseqnum_A;	//acknum expected seqnum으로 재설정
		expectedseqnum_A++;	//expected seqnum +1
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	for (int i = send_base_A; i < nextseqnum_A; i++) //베이스부터 nextseqnum-1까지
	{
		pkt_A[i].acknum = acknum_A;	//acknum재설정
		pkt_A[i].checksum = getCheckSum(&pkt_A[i]);	//이에따른 checksum도 재설정
		printf("A_timerinterrupt, resend packet : [seq : %d] [ack : %d] [data : %s]\n", pkt_A[i].seqnum, pkt_A[i].acknum, pkt_A[i].payload);
		tolayer3(0, pkt_A[i]);	//전부 재전송
	}
	starttimer(0, (float)15);	//그리고 타이머 설정
	printf("A_timerinterrupt, reset timer : [ + %f]\n", (float)15);	//타이머 15초
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	send_base_A = 1;	// 1을 시작점으로 설정
	nextseqnum_A = 1;	
	sequence_A = 1;		 
	expectedseqnum_A = 1;
	acknum_A = 0;	//acknum은 초기에 0으로 설정하여 1을 받지 못한경우를 대비
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	if (packet.checksum != getCheckSum(&packet))	//if pkt corrupted
	{
		printf("B_input packet corrupted : [seq : %d]\n", packet.seqnum);
	}
	else if (packet.seqnum != expectedseqnum_B)	//중복패킷이거나 원하지 않는 패킷
	{
		printf("B_input not expected sequence number : [expected : %d] [seq : %d]\n", expectedseqnum_B, packet.seqnum);
	}
	else {	//패킷 제대로 수신
		printf("B_input receive packet : [seq : %d] [ack : %d] [data : %s]\n", packet.seqnum, packet.acknum, packet.payload);
		tolayer5(1, packet.payload);	//위로 전송
		send_base_B = packet.acknum + 1;	//sendbase재설정
		if (send_base_B == nextseqnum_B) {	//데이터가 없다면 timer멈춤
			stoptimer(1);
		}
		else
			starttimer(1, (float)15);	//나머지의 경우 타이머 재설정
		acknum_B = expectedseqnum_B;	//acknum을 expectednum으로 설정
		expectedseqnum_B++;	//expectednum +1
	}
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
	for (int i = send_base_B; i < nextseqnum_B; i++)	//sendbase부터 nextseqnum -1 까지 전부
	{
		pkt_B[i].acknum = acknum_B;	//acknum재설정
		pkt_B[i].checksum = getCheckSum(&pkt_B[i]);	//이에 따른 checksum도 재설정
		printf("B_timerinterrupt, resend packet : [seq : %d] [ack : %d] [data : %s]\n", pkt_B[i].seqnum, pkt_B[i].acknum, pkt_B[i].payload);
		tolayer3(1, pkt_B[i]);	//재전송
	}
	starttimer(1, (float)15);	//타이머 설정
	printf("B_timerinterrupt, reset timer : [ + %f]\n", (float)15);	//15초로
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	send_base_B = 1;	// 1을 시작점으로 설정
	nextseqnum_B = 1;
	sequence_B = 1;
	expectedseqnum_B = 1;
	acknum_B = 0;	//seqnum1의 데이터를 받기 위해 acknum은 0으로 설정
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
	and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
	interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = (float)0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

void main()
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i, j;
	/* char c; // Unreferenced local variable removed */

	init();
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr == NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist != NULL)
			evlist->prev = NULL;
		if (TRACE >= 2)
		{
			printf("\nEVENT time: %f,", eventptr->evtime);
			printf("  type: %d", eventptr->evtype);
			if (eventptr->evtype == 0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype == 1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n", eventptr->eventity);
		}
		time = eventptr->evtime;        /* update time to next event time */
		if (nsim == nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i = 0; i < 20; i++)
				msg2give.data[i] = 97 + j;
			if (TRACE > 2) {
				printf("          MAINLOOP: data given to student: ");
				for (i = 0; i < 20; i++)
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_output(msg2give);
			else
				B_output(msg2give);
		}
		else if (eventptr->evtype == FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i = 0; i < 20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity == A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype == TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}
terminate:
	printf(" Simulator terminated at time %f\n after sending %d msgsfrom layer5\n", time, nsim);
}



void init()                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();


	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf("%d", &nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf("%f", &lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf("%f", &corruptprob);
	printf("Enter average time between messages from sender's layer5 [ >0.0]:");
	scanf("%f", &lambda);
	printf("Enter TRACE:");
	scanf("%d", &TRACE);

	srand(9999);              /* init random number generator */
	sum = (float)0.0;         /* test random number generator for students */
	for (i = 0; i < 1000; i++)
		sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum / (float)1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n");
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time = (float)0.0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = (float)(rand() / mmm);            /* x should be uniform in [0,1] */
	return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
	double x, log(), ceil();
	struct event *evptr;
	/* char *malloc(); // malloc redefinition removed */
	/* float ttime; // Unreferenced local variable removed */
	/* int tempint; // Unreferenced local variable removed */

	if (TRACE > 2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
							  /* having mean of lambda        */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = (float)(time + x);
	evptr->evtype = FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand() > 0.5))
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}


void insertevent(struct event *p)
{
	struct event *q, *qold;

	if (TRACE > 2) {
		printf("            INSERTEVENT: time is %lf\n", time);
		printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q == NULL) {   /* list is empty */
		evlist = p;
		p->next = NULL;
		p->prev = NULL;
	}
	else {
		for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
			qold = q;
		if (q == NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q == evlist) { /* front of list */
			p->next = evlist;
			p->prev = NULL;
			p->next->prev = p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next = q;
			p->prev = q->prev;
			q->prev->next = p;
			q->prev = p;
		}
	}
}

void printevlist()
{
	struct event *q;
	/* int i; // Unreferenced local variable removed */
	printf("--------------\nEvent List Follows:\n");
	for (q = evlist; q != NULL; q = q->next) {
		printf("Event time: %f, type: %d entity:%d\n", q->evtime, q->evtype, q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
/* A or B is trying to stop timer */
void stoptimer(int AorB)
{
	struct event *q;/* ,*qold; // Unreferenced local variable removed */

	if (TRACE > 2)
		printf("          STOP TIMER: stopping timer at %f\n", time);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
			/* remove this event */
			if (q->next == NULL && q->prev == NULL)
				evlist = NULL;         /* remove first and only event on list */
			else if (q->next == NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q == evlist) { /* front of list - there must be event after */
				q->next->prev = NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next = q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

/* A or B is trying to stop timer */
void starttimer(int AorB, float increment)
{

	struct event *q;
	struct event *evptr;
	/* char *malloc(); // malloc redefinition removed */

	if (TRACE > 2)
		printf("          START TIMER: starting timer at %f\n", time);
	/* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime = (float)(time + increment);
	evptr->evtype = TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}


/************************** TOLAYER3 ***************/
/* A or B is trying to stop timer */
void tolayer3(int AorB, struct pkt packet)
{
	struct pkt *mypktptr;
	struct event *evptr, *q;
	/* char *malloc(); // malloc redefinition removed */
	float lastime, x, jimsrand();
	int i;


	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob) {
		nlost++;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being lost\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i = 0; i < 20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE > 2) {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
			mypktptr->acknum, mypktptr->checksum);
		for (i = 0; i < 20; i++)
			printf("%c", mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
	 medium can not reorder, so make sure packet arrives between 1 and 10
	 time units after the latest arrival time of packets
	 currently in the medium on their way to the destination */
	lastime = time;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q = evlist; q != NULL; q = q->next)
		if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
			lastime = q->evtime;
	evptr->evtime = lastime + 1 + 9 * jimsrand();



	/* simulate corruption: */
	if (jimsrand() < corruptprob) {
		ncorrupt++;
		if ((x = jimsrand()) < .75)
			mypktptr->payload[0] = 'Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE > 0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE > 2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
	int i;
	if (TRACE > 2) {
		printf("          TOLAYER5: data received: ");
		for (i = 0; i < 20; i++)
			printf("%c", datasent[i]);
		printf("\n");
	}

}
