#include <stdlib.h>
#include <stdio.h>

struct twoNums {
	int firstNum;
	int secondNum;
};


struct numberHolder {

	struct twoNums **x; // pointer to a pointer
};

int main(){

	printf("At the start\n");
	// First we have to calloc space for a numberhold struct in the first place
	struct numberHolder *a = (struct numberHolder *)calloc(1, sizeof(struct numberHolder));
	
	// Now we need to allocate space for **x (pointer to a pointer).
	// If we think about it as a 2D array, we are allocating 5 rows right now.
	a->x = (struct twoNums **)calloc(5, sizeof(struct twoNums *));
	printf("We got past calloc\n");

	int i;
	// Now, we are allocating memory for the pointers of the pointers.
	// So now we are adding 3 columns to each row.
	for(i = 0; i < 5; i++){
		a->x[i] = calloc(3, sizeof(struct twoNums));
	}

	printf("We got past calloc for 2D array\n");

	// Just add numbers to test, can be any numbers.
	int j, k;
	struct twoNums l;
	for(j = 0; j < 5; j++){
		for(k = 0; k < 3; k++){
			l.firstNum = j + k;
			l.secondNum = l.firstNum + k;
			a->x[j][k] = l;
		}
	}

	// print out some sample numbers to see if it worked
	printf("firstNum: %d\n", a->x[3][1].firstNum);
	printf("secondNum: %d\n", a->x[4][2].secondNum);

	free(a);
}
