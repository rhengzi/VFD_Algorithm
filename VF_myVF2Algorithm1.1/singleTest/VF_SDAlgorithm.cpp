#include <fstream>
#include <iostream>

#include "argloader.h"
#include "argraph.h"
#include "match.h"
#include "UnixTimer.h"
#include "sd_state.h"

#define MAXNODES 5000

using namespace std;

Timer timer;
int stateCount=0;
int main(int argc, char **argv){
	assert(argc ==3);  // 保证输入参数正确

	// 创建graphA
	ifstream in_A(argv[1], ios::in | ios::binary);
	BinaryGraphLoader loader_A(in_A);
	in_A.close();
	Graph g_A(&loader_A);
	
	// 创建graphB
	ifstream in_B(argv[2], ios::in | ios::binary);
	BinaryGraphLoader loader_B(in_B);
	in_B.close();
	Graph g_B(&loader_B);

	int n;
	node_id *vertices1, *vertices2;
	vertices1 = new node_id[MAXNODES];
	vertices2 = new node_id[MAXNODES];
	// g_A和g_B进行匹配
        timer.start();
	SDState s0(&g_A, &g_B);
        printf("SDState Initial time: %.2fms\n", timer.get_intermediate());
	bool flag = match(&s0, &n, vertices1, vertices2);
        timer.stop();
        printf("SDState number of states: %d\n", stateCount);
        printf("SDState matching time: %.2fms\n", timer.get_duration());
	if(!flag){
			printf("SDState: No matching found!\n");
           
		}
	else{
			printf("SDState: Found a matching with %.d nodes between %s and %s\n", n, argv[1], argv[2]);
	}

	delete [] vertices1;
	delete [] vertices2;
	return 0;
}
