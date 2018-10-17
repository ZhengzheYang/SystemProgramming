#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define BITSPERSEG  (256*sizeof(int) * 8)

typedef struct _seg {  /* definition of new type "seg" */
    int bits[256];
    struct _seg  *next,*prev;
}seg;

seg *head;
void allocateSeg(int N);
seg *whichseg(int j);
int whichint(int j);
int whichbit(int j);
int isPrime(seg *seg, int j);
void setBit(seg *seg, int j);
void markPrimes(int N);
void decompose(int N);

void allocateSeg(int N) {
	seg *pt;
	int	i;
	int odd = (N - 1) / 2;	//convert odd number to index

	int howmany = (odd - 1) / BITSPERSEG + 1;	//get the index of the segment

	head= (  seg * ) malloc(sizeof(seg));
	memset(head, 0, sizeof(seg));
	pt=head;
	for (i=1;i<howmany;i++) {
		pt->next = (  seg *) malloc(sizeof (seg)); 
		memset(pt->next, 0, sizeof(seg));
		pt->next->prev = pt;
		pt = pt->next;
	} 
}

//mark all the bits that represent non-prime numbers to 0
void markPrimes(int N) {
	int count = 0;
	int limit = sqrt(N);
	seg *pt;
	pt = head;

    //no need to traverse to N. sqrt(N) is enough
    for(int i = 3; i <= limit; i = i + 2) {
        if(isPrime(whichseg(i), i)){
            for(int j = 3 * i; j <= N; j = j + 2 * i){
        		setBit(whichseg(j), j);
    		}
        }
    }

	int segIndex = 0;
    while(pt != NULL) {
    	for(int i = 0; i < BITSPERSEG; i++) {
		//if the bit is prime and the number it represents is still in range
    		if((isPrime(pt, i * 2 + 3)) && ((BITSPERSEG * segIndex + i) * 2 + 3) <= N) {
    			count++;
    		}
    	}
    	segIndex++;
    	pt = pt->next;
    }

    printf("%s: %i\n", "The number of odd primes less than or equal to N is", count);

}

//decompose the even number to two prime numbers
seg *prevSeg;
int prevIndex;
int prevEnd;
void decompose(int K) {	//K > 5
	int count = 0;
	int sol = 0;
	int i = 3;
	seg *start, *end;

	start = whichseg(3);	//start from 3
	end = start;
	prevEnd = 0;
	int currentEnd = (K - 6) / 2 / BITSPERSEG;	//find out which segment K-I is in
	for(int j = 0; j < currentEnd; j++) {
		end = end->next;
		prevEnd++;
	}

	while(i <= K / 2) {
		if(isPrime(start, i) & isPrime(end, K - i)) {
			count++;
			sol = i;
		}
		i += 2;
		start = whichseg(i);
		currentEnd = (K - i - 3) / 2 / BITSPERSEG;	//recalculate to find out if the end should move backwards
		while(prevEnd > currentEnd) {
			end = end->prev;
			prevEnd--;
		}
	}
	printf("Largest %i = %i + %i out of %i solutions.\n",  K, sol, K - sol, count);
}

//find out which seg should the parameter be in 
seg *whichseg(int j) {
	j = (j - 3) / 2;
	int whichseg = j / BITSPERSEG;
    if(prevSeg == NULL) {
    	prevSeg = head;
    	for(int i = 0; i < whichseg; i++){
    		prevSeg = prevSeg->next;
    	}
    } else {
	//use the stored segment to go forward or backward
    	if(whichseg > prevIndex) {
    		for(int i = 0; i < (whichseg - prevIndex); i++) {
    			prevSeg = prevSeg->next;
    		}
    	} else if (whichseg < prevIndex) {
    		for(int i = 0; i < (prevIndex - whichseg); i++) {
    			prevSeg = prevSeg->prev;
    		}
    	}
    }
	prevIndex = whichseg;
	return prevSeg;
}

int whichint(int j) {
	j = (j - 3) / 2;
	int whichint = j % BITSPERSEG / (sizeof(int) * 8);
    return whichint;
}

int whichbit(int j) {
	j = (j - 3) / 2;
	int whichbit = j % BITSPERSEG % (sizeof(int) * 8);
    return whichbit;
}

//check primality
int isPrime(seg *seg, int j) {
    if((seg->bits[whichint(j)] & (1 << whichbit(j))) != 0)
        return 0;
    else
        return 1;
}

//set the bit to 1 using bit manipulation
void setBit(seg *seg, int j) {
    seg->bits[whichint(j)] |= (1 << whichbit(j));
}

int main(int argc, char *argv[]) {
    int N = atoi(argv[1]);
    allocateSeg(N);
    markPrimes(N);

    int K;
	printf("%s\n", "Enter Even Numbers >5 for Goldbach Tests");
	while(1) {
		if(scanf("%d", &K) == 0 || feof(stdin)) break;	//detect EOF or unwanted input such as arrows or crl on keyboard
    	if(K % 2 != 0 || K > N || K < 5) {
    		printf("%s\n", "Please enter a valid input");
    		continue;
    	}
    	decompose(K);
        printf("%s\n", "Please input another:");
    }
    return 0;
}

