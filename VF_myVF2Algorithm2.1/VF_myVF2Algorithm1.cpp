#include <fstream>
#include <iostream>
#include <string>

#include <stdio.h>
#include <string.h>

#include "argloader.h"
#include "argraph.h"
#include "match.h"
#include "UnixTimer.h"
#include "my_vf2_state.h"

#define MAXNODES 5000

using namespace std;

Timer timer;
long stateCount=0;
long nodesCount=0;
ofstream TimeIn;

void singleTest(char* fileA, char* fileB){

        // create graphA
	ifstream in_A(fileA, ios::in | ios::binary);
	BinaryGraphLoader loader_A(in_A);
	in_A.close();
	Graph g_A(&loader_A);
	
	// create graphB
	ifstream in_B(fileB, ios::in | ios::binary);
	BinaryGraphLoader loader_B(in_B);
	in_B.close();
	Graph g_B(&loader_B);

	int n;
	node_id *vertices1, *vertices2;
	vertices1 = new node_id[MAXNODES];
	vertices2 = new node_id[MAXNODES];

        // write the number of nodes and edges into TimeIn
        int num_edges=0;
        for(int i=0; i<g_A.NodeCount(); i++){
            num_edges += g_A.EdgeCount(i);
        }
        TimeIn<<g_A.NodeCount()<<"\t"<<num_edges<<"\t";
	// g_A and g_B begin matching
        timer.start();
	MyVF2State s0(&g_A, &g_B);
        TimeIn<<timer.get_intermediate()<<"\t";
        //printf("SDState Initial time: %.5fs\n", timer.get_intermediate());
	bool flag = match(&s0, &n, vertices1, vertices2);
        timer.stop();
        TimeIn<<timer.get_duration()<<"\t";
        //printf("SDState matching time: %.5fs\n", timer.get_duration());
        TimeIn<<stateCount<<"\t";
        //printf("SDState number of states: %d\n", stateCount);
        TimeIn<<nodesCount<<"\t";
	if(!flag){
			//printf("SDState: No matching found!\n");
                        TimeIn<<0<<endl;
           
		}
	else{
			//printf("SDState: Found a matching with %.d nodes between %s and %s\n", n, fileA, fileB);
                        TimeIn<<1<<endl;
	}

	delete [] vertices1;
	delete [] vertices2;
}

void mulTest(char *fileListA, char *fileListB){
    ifstream fileListInA(fileListA);
    ifstream fileListInB(fileListB);

    assert(fileListInA.is_open());
    assert(fileListInB.is_open());
    
    string fileNameA, fileNameB; 
    char fileA[100], fileB[100];
    int cnt=0;  
    while(getline(fileListInA, fileNameA) && getline(fileListInB, fileNameB)){
        cnt++;
        strcpy(fileA, fileNameA.data());
        strcpy(fileB, fileNameB.data());
        printf("matching %dth between %s and %s\n", cnt, fileA, fileB); 
        stateCount=0;
        nodesCount=0; 
        singleTest(fileA, fileB);

    }
 
}

int main(int argc, char **argv){
    assert(argc==4);

    char *fileListA, *fileListB, *timeFile;
    
    fileListA = argv[1];
    fileListB = argv[2];
    timeFile = argv[3];

//	fileListA = "fileListA";
 //   fileListB = "fileListB";
  //  timeFile = "time.txt";
   
    TimeIn.open(timeFile, ios::out);
    int resultHeaderNum=7;
    char* resultHeader[] = {"nodes", "edges", "initialState", "matchTime", "numberOfVF2State", "numberOfExploreNodes", "flag"};
    for(int i=0; i<resultHeaderNum; i++){
        if(i!=resultHeaderNum-1)
            TimeIn<<resultHeader[i]<<"\t";
        else
            TimeIn<<resultHeader[i]<<endl;
    }
    mulTest(fileListA, fileListB);
    TimeIn.close();
    return 0;
}
