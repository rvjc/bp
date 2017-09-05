#include <windows.h>
#include <stdio.h>
#include <sys/time.h> 
#include <stdlib.h>
#include <math.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

#define NUMSET_SIZE 40
#define MAX_PERMS 12157665459056928801ULL
#define MAX_DIGITS 20
#define LONG_DIGITS 17
#define SHORT_DIGITS 3

////////////////////////////////////////////////////////////////////////////////

char numstrings[NUMSET_SIZE][LONG_DIGITS+1] = {
	"01961326924040293",
	"02761700544988444",
	"05200868530312338",
	"06733469273599763",
	"07405243305034455",
	"08700437200878410",
	"10620425258631077",
	"12299818124503999",
	"13224355562020053",
	"13612180134591204",
	"15203535578407617",
	"18379966372564246",
	"32141343228723542",
	"32359498454675805",
	"34392448780418257",
	"34767834185807748",
	"36997670336636856",
	"38512447624065660",
	"40531327219709413",
	"47754623452792821",
	"51288427274159783",
	"58705699231578517",
	"67056842959265980",
	"68142821209791299",
	"69093267507519956",
	"70401878668986612",
	"71840670501564979",
	"74677056107290551",
	"76593737631941957",
	"77982002763709258",
	"79735075169433456",
	"80004341625281791",
	"81823684067586519",
	"83140484899395946",
	"85182045204745059",
	"86772047279382738",
	"89369975981769402",
	"90440805781442827",
	"92981284945048715",
	"96304218072759657"
};

////////////////////////////////////////////////////////////////////////////////

typedef long long int64;
typedef unsigned long long uint64;

////////////////////////////////////////////////////////////////////////////////

int64 strval(const char* s) {
	return strtoull(s,0,10);
};

////////////////////////////////////////////////////////////////////////////////

const char* valstr(int64 v) {
	
	static char s[MAX_DIGITS+1];
	
	snprintf(s, MAX_DIGITS, "%I64u", v);
	return s;
};

////////////////////////////////////////////////////////////////////////////////

typedef struct {
	int64 	val;		// high-precision value
	int 	pos;		// 0=absent, +1=left, -1=right
	int64 	dif;		// dif of remaining values from current index
	int64 	sum;		// sum of remaining values from current index
	int64 	max;		// max positive shift for remaining values from current index
	int64	min;		// min negative shift for remaining values from current index
	uint64	per;    	// remaining permutations from current index 
} number;

number numset[NUMSET_SIZE];

////////////////////////////////////////////////////////////////////////////////

time_t	started;
uint64	tested;
uint64	skipped;
int64 	bestdifpos;
int64	bestdifneg;
int64	lastdif;
int64 	lastdifabs;
int 	leftsize;
int 	rightsize;

////////////////////////////////////////////////////////////////////////////////

void swap(int i, int j) {
	
	int64 v;
	
	if (i != j) {
		v = numset[i].val;
		numset[i].val = numset[j].val;
		numset[j].val = v;
	};
};

////////////////////////////////////////////////////////////////////////////////

const char* shortform(int64 val) {
	
	// Returns rounded shortform string with leading zeroes where applicable
			
	static char s[SHORT_DIGITS+1];
	
	snprintf(s, SHORT_DIGITS, "%0*d", SHORT_DIGITS, int(round(double(val)/pow(10.0, LONG_DIGITS-SHORT_DIGITS))));
	return s;
};

////////////////////////////////////////////////////////////////////////////////

const char* longform(int64 val) {
	
	// Returns fixed-width full precision string with leading zeroes where applicable 
	
	unsigned int i;
	const char *t;
	static char s[LONG_DIGITS+1];
	
	t = valstr(val);
	
	for (i=0; i < LONG_DIGITS - strlen(t); i++) {
		s[i] = '0';
	}
	strncpy (s+i, t, LONG_DIGITS);
	return s;
};

////////////////////////////////////////////////////////////////////////////////

char poschar(int pos) {
	if (pos > 0) return '+'; else if (pos < 0) return '-'; else return ' ';
};

////////////////////////////////////////////////////////////////////////////////

void seed() {

	struct timeval t1, t2;
	unsigned int s;
	
	// construct seed from precise (but non-standard) gettimeofday()
	// bear in mind that microseconds field only has 1mS precision
	// so combine two samples with time between samples determined by user
	
	gettimeofday(&t1, NULL); 
	
	printf("Press ENTER when you're feeling lucky\n");
	getchar();
	
	gettimeofday(&t2, NULL); 
	
	s = 1000 * (t1.tv_usec/1000) + t2.tv_usec/1000;
	printf("Seed: %u\n", s);
	srand(s);
};

////////////////////////////////////////////////////////////////////////////////

void init() {
	
	int i, j, r;
	number* n;
	int64 sum, dif;
	uint64 per;
	
	started = time(0);
	tested = 0ULL;
	skipped = 0ULL;
	bestdifpos = 0LL;
	bestdifneg = 0LL;
	lastdif = 0LL;
	lastdifabs = 0LL;
	leftsize = 0LL;
	rightsize = 0LL;
	
	// convert string values
	
	for (i=0; i < NUMSET_SIZE; i++) {
		n = &numset[i];
		n->val = strval(numstrings[i]);
	};
	
	// sort values in descending order
	// this maximises skipping rate
	
	for (i=0; i < NUMSET_SIZE; i++) {
		for (j=i+1; j < NUMSET_SIZE; j++) {
			if (numset[j].val > numset[i].val) {
				swap(i,j);
			};
		};
	};
	
	// seed randomiser from system clock
	
	seed();
	
	// randomise positions at each index
	
	for (i=0; i < NUMSET_SIZE; i++) {
		
		n = &numset[i];
		r = rand() % 3;
		
		switch (r) {
			
			case 0:
				n->pos = 0;
				break;
				
			case 1:
				n->pos = +1;
				leftsize += 1;
				lastdif += n->val;
				break;
				
			case 2:
				n->pos = -1;
				rightsize += 1;
				lastdif -= n->val;
				break;
		};
	};
	
	lastdifabs = llabs(lastdif);
	bestdifpos = +lastdifabs;
	bestdifneg = -lastdifabs;
	
	// pre-calculate dif, sum, min, max, per
	// of remaining values from each index
	
	for (i=0; i < NUMSET_SIZE; i++) {
		
		dif = 0LL;
		sum = 0LL;
		per = 1ULL;

		for (j=i; j < NUMSET_SIZE; j++) {
			
			n = &numset[j];
			
			switch (n->pos) {
				case 0:
					break;
				case +1:
					dif += n->val;
					break;
				case -1:
					dif -= n->val;
					break;
			};
			
			sum += n->val;
			per *= 3;
		};
		
		n = &numset[i];
		
		n->dif = dif;
		n->sum = sum;
		n->max = sum - dif;
		n->min = sum + dif;
		n->per = per;
	};
};

////////////////////////////////////////////////////////////////////////////////

void printTable() {
	int i;
	number n;
	
	printf("\n");
	printf("===============================================================================\n");
	printf("VAL   64-BIT PRECISION  SUM        DIF       MIN       MAX       PER\n");
	printf("-------------------------------------------------------------------------------\n");
	
	for (i=0; i < NUMSET_SIZE; i++) {
		n = numset[i];
		printf("%s %c %*s %.2e %+.2e %.2e %.2e %1.0e\n",
			   shortform(n.val),
			   poschar(n.pos),
			   LONG_DIGITS,
			   longform(n.val),
			   double(n.sum),
			   double(n.dif),
			   double(n.min),
			   double(n.max),
			   double(n.per));
	};
};

////////////////////////////////////////////////////////////////////////////////

void printLog() {
	
	int 		i, j;
	number  	n;
	long		timedif, hours, minutes, seconds;
	uint64  	covered;
	
	timedif = long(difftime(time(0), started));
	seconds = timedif % 60;
	minutes = (timedif/60) % 60;
	hours = (timedif/3600) % 60;
	
	covered = tested + skipped;
	
	printf("\n");
	printf("===============================================================================\n\n");
	printf("DIFF (64)  : %s\n\n", valstr(lastdifabs));
	printf("COVERED    : %I64u of %I64u\n\n", covered, MAX_PERMS);
	printf("TESTED     : %I64u\n\n", tested);
	printf("SKIPPED    : %I64u\n\n", skipped);
	printf("TIME       : %ld hours, %ld mins, %ld secs\n\n", hours, minutes, seconds);
	
	printf("LEFT  (%02d) : ", leftsize);
	j = 0;
	for (i=0; i < NUMSET_SIZE; i++) {
		n = numset[i];
		if (n.pos == 1) {
			if (j == 10) {
				j = 0;
				printf("\n             ");
			} else {
				j += 1;
			};
			printf("%s, ", shortform(n.val));
		};
	};
	printf("\n\n");
	
	printf("RIGHT (%02d) : ", rightsize);
	j = 0;
	for (i=0; i < NUMSET_SIZE; i++) {
		n = numset[i];
		if (n.pos == -1) {
			if (j == 10) {
				j = 0;
				printf("\n             ");
			} else {
				j += 1;
			};
			printf("%s, ", shortform(n.val));
		};
	};
	printf("\n");
};

////////////////////////////////////////////////////////////////////////////////

void test(int i) {
	
	// this function is recursive
	// i argument is index of current number
	// next recursion level is at next number
	// 3 position possibilities for each number
	// total permutations = 3 ^ NUMSET_SIZE
	// if NUMSET_SIZE = 40, this works out at 1.2E19 permutations!!!
	// however, many of these permutations can be skipped
	
	number* n;	// current number
	int64	v; 	// current number value
	int		p;  // current number position

	if (i == NUMSET_SIZE) {
		
		// lowest recursion level
		// test current permutation and return
		
		tested += 1;
		
		if (lastdifabs < bestdifpos) {
			if (leftsize || rightsize) {
				bestdifpos = +lastdifabs;
				bestdifneg = -lastdifabs;
				printLog();
			};
		};
		return;
	};
	
	n = &numset[i];
	
	if (lastdif < 0LL) {
		if ((lastdif + n->max) < bestdifneg) {
			skipped += n->per;
			return;
		};
	} else { 
		if ((lastdif - n->min) > bestdifpos) {
			skipped += n->per;
			return;
		};
	};
	
	// recurse with three permutations of current number position
	// before returning position to its original state
	
	v = n->val;
	p = n->pos;
	
	switch (p) {
		
		case 0:
		
			test(i+1);
	
			n->pos = +1;
			leftsize += 1;
			lastdif += v;
			lastdifabs = llabs(lastdif);
			
			test(i+1);
			
			n->pos = -1;
			leftsize -= 1;
			rightsize += 1;
			lastdif -= v; lastdif -= v;
			lastdifabs = llabs(lastdif);
			
			test(i+1);
			
			n->pos = 0;
			rightsize -= 1;
			lastdif += v;
			lastdifabs = llabs(lastdif);
			
			break;
		
		case +1:
		
			test(i+1);
			
			n->pos = -1;
			leftsize -= 1;
			rightsize += 1;
			lastdif -= v; lastdif -= v;
			lastdifabs = llabs(lastdif);
			
			test(i+1);
			
			n->pos = 0;
			rightsize -= 1;
			lastdif += v;
			lastdifabs = llabs(lastdif);
			
			test(i+1);
	
			n->pos = +1;
			leftsize += 1;
			lastdif += v;
			lastdifabs = llabs(lastdif);
			
			break;
		
		case -1:
		
			test(i+1);
			
			n->pos = +1;
			leftsize += 1;
			rightsize -= 1;
			lastdif += v; lastdif += v;
			lastdifabs = llabs(lastdif);
			
			test(i+1);
			
			n->pos = 0;
			leftsize -= 1;
			lastdif -= v;
			lastdifabs = llabs(lastdif);
			
			test(i+1);
	
			n->pos = -1;
			rightsize += 1;
			lastdif -= v;
			lastdifabs = llabs(lastdif);
			
			break;
	};
};

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
	
    int64 param;
	
	init();
	Sleep(2000);
	
	printTable();
	Sleep(2000);
	
	printLog();
	
	param = 0;
	if (argc == 2) {
		param = strval(argv[1]);
	};
	
	// A single program parameter is interpreted as "the value to beat".
	// This should be the "best difference" obtained from a previous run.
	// As this parameter decreases, the initial skipping rate will increase. 
	// This will speed up test coverage until the value is beaten,
	// at which point, the program will settle down as normal.
    
	if (param > 0) {
		printf("\n");
		printf("===============================================================================\n\n");
		printf("TO BEAT    : %s\n", valstr(param));
		bestdifpos = +param;
		bestdifneg = -param;
	};
	
	test(0);
	
	printf("\n");
	printf("===============================================================================\n\n");
	printf("All permutations covered!\n\n");
	printf("How did you live that long?\n\n");
	printf("If you're still alive, press ENTER to exit:\n");
	getchar();

	return 0;
};

////////////////////////////////////////////////////////////////////////////////