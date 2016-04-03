#include <stdio.h>
#include <stdlib.h>
enum example{
	type1,
	type2
};

struct a{
	enum example e;
};

int main(){
	struct a yo;
	yo.e = type1;

	if(yo.e == type2){
		printf("This should not happen\n");
	}
	else{
		printf("You should see this\n");
	}
}
