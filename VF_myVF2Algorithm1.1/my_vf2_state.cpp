/*------------------------------------------------------------------
 * vf2_state.cc
 * Implementation of the class VF2State
 *
 * Author: P. Foggia
 *-----------------------------------------------------------------*/



/*-----------------------------------------------------------------
 * NOTE: 
 *   The attribute compatibility check (methods CompatibleNode
 *   and CompatibleEdge of ARGraph) is always performed
 *   applying the method to g1, and passing the attribute of
 *   g1 as first argument, and the attribute of g2 as second
 *   argument. This may be important if the compatibility
 *   criterion is not symmetric.
 -----------------------------------------------------------------*/


/*---------------------------------------------------------
 *   IMPLEMENTATION NOTES:
 * The six vectors core_1, core_2, in_1, in_2, out_1, out_2, 
 * are shared among the instances of this class; they are
 * owned by the instance with core_len==0 (the root of the
 * SSR).
 * In the vectors in_* and out_* there is a value indicating 
 * the level at which the corresponding node became a member
 * of the core or of Tin (for in_*) or Tout (for out_*),
 * or 0 if the node does not belong to the set.
 * This information is used for backtracking.
 * The fields t1out_len etc. also count the nodes in core.
 * The true t1out_len is t1out_len-core_len!
 ---------------------------------------------------------*/


#include <stddef.h>
#include <queue>

#include "my_vf2_state.h"

#include "error.h"

#include "sortnodes.h"

using namespace std;

static node_id **create_Kthdistance(Graph *g, int k);
static void createKthdistance(Graph *g,node_id** d, int k, node_id node0);
static node_id** raw_charac(node_id **dist, int n);
static node_id** col_charac(node_id **dist, int n);

static bool compare_array(node_id *array1, node_id *array2, int n);
static void compute_initial_partition(node_id **dist1, node_id **dist2,
	                              node_id *cls1, node_id *cls2, int n);
static void initial_cls(node_id* cls1, node_id* cls2, node_id** raw_charac1,\
				node_id** raw_charac2, node_id** col_charac1, node_id** col_charac2,
				int n);


/*----------------------------------------------------------
 * Methods of the class VF2State
 ---------------------------------------------------------*/

/*----------------------------------------------------------
 * VF2State::VF2State(g1, g2)
 * Constructor. Makes an empty state.
 ---------------------------------------------------------*/
MyVF2State::MyVF2State(Graph *ag1, Graph *ag2, bool sortNodes)
  { g1=ag1;
    g2=ag2;
    n1=g1->NodeCount();
    n2=g2->NodeCount();

    if (sortNodes)
      order=SortNodesByFrequency(ag1);
    else
      order=NULL;

    core_len=orig_core_len=0;
    t1both_len=t1in_len=t1out_len=0;
    t2both_len=t2in_len=t2out_len=0;

	added_node1=NULL_NODE;

    core_1=new node_id[n1];
    core_2=new node_id[n2];
    in_1=new node_id[n1];
    in_2=new node_id[n2];
    out_1=new node_id[n1];
    out_2=new node_id[n2];
	share_count = new long;

    if (!core_1 || !core_2 || !in_1 || !in_2 
	    || !out_1 || !out_2 || !share_count)
      error("Out of memory");

    int i;
    for(i=0; i<n1; i++)
      { 
        core_1[i]=NULL_NODE;
		in_1[i]=0;
		out_1[i]=0;
		
      }
    for(i=0; i<n2; i++)
      { 
        core_2[i]=NULL_NODE;
		in_2[i]=0;
		out_2[i]=0;
      }
	
	*share_count = 1;
	 node_id **dist1, **dist2;
		 
	if(n1==n2){
		features1=new node_id[n1];
		features2=new node_id[n2];
		dist1 = create_Kthdistance(g1, n1);
		dist2 = create_Kthdistance(g2,n2);
		for(int i=0; i<n1; i++){
		   features1[i] = 0;
		   features2[i] = 0;
		}
		compute_initial_partition(dist1, dist2, features1, features2, n1); 
		for(int i=0; i<n1; i++){
		   delete [] dist1[i];
		   delete [] dist2[i];
		
	    }

	    delete [] dist1;
	    delete[] dist2;
	}
	
  }


/*----------------------------------------------------------
 * VF2State::VF2State(state)
 * Copy constructor. 
 ---------------------------------------------------------*/
MyVF2State::MyVF2State(const MyVF2State &state)
  { g1=state.g1;
    g2=state.g2;
    n1=state.n1;
    n2=state.n2;

    order=state.order;

    core_len=orig_core_len=state.core_len;
    t1in_len=state.t1in_len;
    t1out_len=state.t1out_len;
    t1both_len=state.t1both_len;
    t2in_len=state.t2in_len;
    t2out_len=state.t2out_len;
    t2both_len=state.t2both_len;
	if(n1==n2){
        features1 = state.features1;
        features2 = state.features2;
	}
    added_node1=NULL_NODE;

    core_1=state.core_1;
    core_2=state.core_2;
    in_1=state.in_1;
    in_2=state.in_2;
    out_1=state.out_1;
    out_2=state.out_2;
    share_count=state.share_count;

	++ *share_count;

  }


/*---------------------------------------------------------------
 * VF2State::~VF2State()
 * Destructor.
 --------------------------------------------------------------*/
MyVF2State::~MyVF2State() 
  { if (-- *share_count == 0)
    { delete [] core_1;
      delete [] core_2;
      delete [] in_1;
      delete [] out_1;
      delete [] in_2;
      delete [] out_2;
      delete share_count;
	  if(n1==n2){
		delete [] features1;
		delete [] features2;
      }
      delete [] order;
	}
  }


/*--------------------------------------------------------------------------
 * bool VF2State::NextPair(pn1, pn2, prev_n1, prev_n2)
 * Puts in *pn1, *pn2 the next pair of nodes to be tried.
 * prev_n1 and prev_n2 must be the last nodes, or NULL_NODE (default)
 * to start from the first pair.
 * Returns false if no more pairs are available.
 -------------------------------------------------------------------------*/

bool MyVF2State::NextPair(node_id *pn1, node_id *pn2,
              node_id prev_n1, node_id prev_n2)
  { 
    if (prev_n1==NULL_NODE)
      prev_n1=0;

    if (prev_n2==NULL_NODE)
      prev_n2=0;
    else
      prev_n2++;

	if (t1both_len>core_len && t2both_len>core_len)
	  { while (prev_n1<n1 &&
           (core_1[prev_n1]!=NULL_NODE || out_1[prev_n1]==0
		            || in_1[prev_n1]==0))
          { prev_n1++;    
            prev_n2=0;
          }
	  }
	else if (t1out_len>core_len && t2out_len>core_len)
	  { while (prev_n1<n1 &&
           (core_1[prev_n1]!=NULL_NODE || out_1[prev_n1]==0))
          { prev_n1++;    
            prev_n2=0;
          }
	  }
    else if (t1in_len>core_len && t2in_len>core_len)
	  { while (prev_n1<n1 &&
           (core_1[prev_n1]!=NULL_NODE || in_1[prev_n1]==0))
          { prev_n1++;    
            prev_n2=0;
          }
	  }
	else if (prev_n1==0 && order!=NULL)
	  { int i=0;
	    while (i<n1 && core_1[prev_n1=order[i]]!=NULL_NODE)
	      i++;
	    if (i==n1)
	      prev_n1=n1;
	  }
	else
	  { while (prev_n1<n1 && core_1[prev_n1]!=NULL_NODE )
          { prev_n1++;    
            prev_n2=0;
          }
	  }


	if (t1both_len>core_len && t2both_len>core_len)
	  { while (prev_n2<n2 &&
           (core_2[prev_n2]!=NULL_NODE || out_2[prev_n2]==0
		            || in_2[prev_n2]==0 ||features1[prev_n1]!=features2[prev_n2]))
          { prev_n2++;    
          }
	  }
	else if (t1out_len>core_len && t2out_len>core_len)
	  { while (prev_n2<n2 &&
           (core_2[prev_n2]!=NULL_NODE || out_2[prev_n2]==0 || features1[prev_n1]!=features2[prev_n2]))
          { prev_n2++;    
          }
	  }
    else if (t1in_len>core_len && t2in_len>core_len)
	  { while (prev_n2<n2 &&
           (core_2[prev_n2]!=NULL_NODE || in_2[prev_n2]==0 || features1[prev_n1]!=features2[prev_n2]))
          { prev_n2++;    
          }
	  }
	else
	  { while (prev_n2<n2 && (core_2[prev_n2]!=NULL_NODE || features1[prev_n1]!=features2[prev_n2]))
          { prev_n2++;    
          }
	  }
	  

    if (prev_n1<n1 && prev_n2<n2)
          { *pn1=prev_n1;
            *pn2=prev_n2;
            return true;
          }

    return false;
  }

/*---------------------------------------------------------------
 * bool VF2State::IsFeasiblePair(node1, node2)
 * Returns true if (node1, node2) can be added to the state
 * NOTE: 
 *   The attribute compatibility check (methods CompatibleNode
 *   and CompatibleEdge of ARGraph) is always performed
 *   applying the method to g1, and passing the attribute of
 *   g1 as first argument, and the attribute of g2 as second
 *   argument. This may be important if the compatibility
 *   criterion is not symmetric.
 --------------------------------------------------------------*/

bool MyVF2State::IsFeasiblePair(node_id node1, node_id node2)
  { assert(node1<n1);
    assert(node2<n2);
    assert(core_1[node1]==NULL_NODE);
    assert(core_2[node2]==NULL_NODE);

    if (!g1->CompatibleNode(g1->GetNodeAttr(node1), g2->GetNodeAttr(node2)))
      return false;
	
    int i, other1, other2;
    void *attr1;
    int termout1out=0, termout2out=0, termin1out=0, termin2out=0, termout1in=0, termout2in=0, termin1in=0, termin2in=0;
	
	if(g1->OutEdgeCount(node1)!=g2->OutEdgeCount(node2) || g1->InEdgeCount(node1)!=g2->InEdgeCount(node2))
		return false;
    // Check the 'out' edges of node1
    for(i=0; i<g1->OutEdgeCount(node1); i++)
      { other1=g1->GetOutEdge(node1, i, &attr1);
        if (core_1[other1] != NULL_NODE)
          { other2=core_1[other1];
            if (!g2->HasEdge(node2, other2) ||
                !g1->CompatibleEdge(attr1, g2->GetEdgeAttr(node2, other2)))
              return false;
          }
        else 
          { if (in_1[other1])
              termin1out++;
            if (out_1[other1])
              termout1out++;
           // if (!in_1[other1] && !out_1[other1])
            //  newout1++;
          }
      }

    // Check the 'in' edges of node1
    for(i=0; i<g1->InEdgeCount(node1); i++)
      { other1=g1->GetInEdge(node1, i, &attr1);
        if (core_1[other1]!=NULL_NODE)
          { other2=core_1[other1];
            if (!g2->HasEdge(other2, node2) ||
                !g1->CompatibleEdge(attr1, g2->GetEdgeAttr(other2, node2)))
              return false;
          }
        else 
          { if (in_1[other1])
              termin1in++;
            if (out_1[other1])
              termout1in++;
            //if (!in_1[other1] && !out_1[other1])
             // newin1++;
          }
      }


    // Check the 'out' edges of node2
    for(i=0; i<g2->OutEdgeCount(node2); i++)
      { other2=g2->GetOutEdge(node2, i);
        if (core_2[other2]!=NULL_NODE)
          { other1=core_2[other2];
            if (!g1->HasEdge(node1, other1))
              return false;
          }
        else 
          { if (in_2[other2])
              termin2out++;
            if (out_2[other2])
              termout2out++;
           // if (!in_2[other2] && !out_2[other2])
            //  newout2++;
          }
      }

    // Check the 'in' edges of node2
    for(i=0; i<g2->InEdgeCount(node2); i++)
      { other2=g2->GetInEdge(node2, i);
        if (core_2[other2] != NULL_NODE)
          { other1=core_2[other2];
            if (!g1->HasEdge(other1, node1))
              return false;
          }
        else 
          { if (in_2[other2])
              termin2in++;
            if (out_2[other2])
              termout2in++;
           // if (!in_2[other2] && !out_2[other2])
           //   newin2++;
          }
      }

    return  termin1out==termin2out && termout1out==termout2out && termout1in==termout2in &&termin1in==termin2in;
  }
  
/*--------------------------------------------------------------
 * void VF2State::AddPair(node1, node2)
 * Adds a pair to the Core set of the state.
 * Precondition: the pair must be feasible
 -------------------------------------------------------------*/
void MyVF2State::AddPair(node_id node1, node_id node2)
  { assert(node1<n1);
    assert(node2<n2);
    assert(core_len<n1);
    assert(core_len<n2);

    core_len++;
	added_node1=node1;

	if (!in_1[node1])
	  { in_1[node1]=core_len;
	    t1in_len++;
		if (out_1[node1])
		  t1both_len++;
	  }
	if (!out_1[node1])
	  { out_1[node1]=core_len;
	    t1out_len++;
		if (in_1[node1])
		  t1both_len++;
	  }

	if (!in_2[node2])
	  { in_2[node2]=core_len;
	    t2in_len++;
		if (out_2[node2])
		  t2both_len++;
	  }
	if (!out_2[node2])
	  { out_2[node2]=core_len;
	    t2out_len++;
		if (in_2[node2])
		  t2both_len++;
	  }

    core_1[node1]=node2;
    core_2[node2]=node1;


    int i, other;
    for(i=0; i<g1->InEdgeCount(node1); i++)
      { other=g1->GetInEdge(node1, i);
        if (!in_1[other])
          { in_1[other]=core_len;
            t1in_len++;
		    if (out_1[other])
		      t1both_len++;
          }
      }

    for(i=0; i<g1->OutEdgeCount(node1); i++)
      { other=g1->GetOutEdge(node1, i);
        if (!out_1[other])
          { out_1[other]=core_len;
            t1out_len++;
		    if (in_1[other])
		      t1both_len++;
          }
      }
    
    for(i=0; i<g2->InEdgeCount(node2); i++)
      { other=g2->GetInEdge(node2, i);
        if (!in_2[other])
          { in_2[other]=core_len;
            t2in_len++;
		    if (out_2[other])
		      t2both_len++;
          }
      }

    for(i=0; i<g2->OutEdgeCount(node2); i++)
      { other=g2->GetOutEdge(node2, i);
        if (!out_2[other])
          { out_2[other]=core_len;
            t2out_len++;
		    if (in_2[other])
		      t2both_len++;
          }
      }

  }



/*--------------------------------------------------------------
 * void VF2State::GetCoreSet(c1, c2)
 * Reads the core set of the state into the arrays c1 and c2.
 * The i-th pair of the mapping is (c1[i], c2[i])
 --------------------------------------------------------------*/
void MyVF2State::GetCoreSet(node_id c1[], node_id c2[])
  { int i,j;
    for (i=0,j=0; i<n1; i++)
      if (core_1[i] != NULL_NODE)
        { c1[j]=i;
          c2[j]=core_1[i];
          j++;
        }
  }


/*----------------------------------------------------------------
 * Clones a VF2State, allocating with new the clone.
 --------------------------------------------------------------*/
State* MyVF2State::Clone()
  { return new MyVF2State(*this);
  }

/*----------------------------------------------------------------
 * Undoes the changes to the shared vectors made by the 
 * current state. Assumes that at most one AddPair has been
 * performed.
 ----------------------------------------------------------------*/
void MyVF2State::BackTrack()
  { assert(core_len - orig_core_len <= 1);
    assert(added_node1 != NULL_NODE);
  
    if (orig_core_len < core_len)
      { int i, node2;

        if (in_1[added_node1] == core_len)
		  in_1[added_node1] = 0;
	    for(i=0; i<g1->InEdgeCount(added_node1); i++)
		  { int other=g1->GetInEdge(added_node1, i);
		    if (in_1[other]==core_len)
			  in_1[other]=0;
		  }
        
		if (out_1[added_node1] == core_len)
		  out_1[added_node1] = 0;
	    for(i=0; i<g1->OutEdgeCount(added_node1); i++)
		  { int other=g1->GetOutEdge(added_node1, i);
		    if (out_1[other]==core_len)
			  out_1[other]=0;
		  }
	    
		node2 = core_1[added_node1];

        if (in_2[node2] == core_len)
		  in_2[node2] = 0;
	    for(i=0; i<g2->InEdgeCount(node2); i++)
		  { int other=g2->GetInEdge(node2, i);
		    if (in_2[other]==core_len)
			  in_2[other]=0;
		  }
        
		if (out_2[node2] == core_len)
		  out_2[node2] = 0;
	    for(i=0; i<g2->OutEdgeCount(node2); i++)
		  { int other=g2->GetOutEdge(node2, i);
		    if (out_2[other]==core_len)
			  out_2[other]=0;
		  }
	    
	    core_1[added_node1] = NULL_NODE;
		core_2[node2] = NULL_NODE;
	    
	    core_len=orig_core_len;
		added_node1 = NULL_NODE;
	  }

  }

/*---------------------------------------------------------
 * Static functions
 --------------------------------------------------------*/

static void compute_initial_partition(node_id **dist1, node_id **dist2,
	                              node_id *cls1, node_id *cls2, int n)
   {
	node_id **raw_charac1, **raw_charac2, **col_charac1, **col_charac2;
	// computing the character matrix of dist
	raw_charac1 = raw_charac(dist1, n);
	raw_charac2 = raw_charac(dist2, n);
	col_charac1 = col_charac(dist1, n);
	col_charac2 = col_charac(dist2, n);
	// computing the cls of nodes
	initial_cls(cls1, cls2, raw_charac1, raw_charac2, col_charac1, col_charac2, n);

    }

static node_id **create_Kthdistance(Graph *g, int k){
    int n=g->NodeCount();

    assert(n < NULL_NODE);

    node_id **d;
    d=new node_id *[n];
    //bool *knowns;
    //knowns = new bool [n];
    //for(int i=0; i<n; i++)
     //   knowns[i] = false;
    for(int i=0; i<n; i++){
      d[i]=new node_id[n];
	  for(int j=0; j<n; j++)
        d[i][j]=NULL_NODE;
	}
	for(int i=0; i<n; i++){
		createKthdistance(g,d, k, i);
	}
	return d;
}

static void createKthdistance(Graph *g,node_id** d, int k, node_id node0){
	queue<node_id > q;
	q.push(node0);
	d[node0][node0]=0;
	node_id node1,node2;
	int edgesNum;
	while(!q.empty()){
		node1=q.front();
		q.pop();
		//knowns[node1] = true;
		edgesNum = g->OutEdgeCount(node1);
		for(int i=0;i<edgesNum; i++){
			node2 = g->GetOutEdge(node1,i);
			//if(d[node0][node2]<k){
				if(d[node0][node2]==NULL_NODE){
                    d[node0][node2] = d[node0][node1]+1;
                    if(d[node0][node2]<k)
				      q.push(node2);
				}


			//}
		}

	}
}

static node_id **create_distance_matrix(Graph *g){
	int i, j, k;
    int n=g->NodeCount();

    assert(n < NULL_NODE);

    node_id **d;
    d=new node_id *[n];
    for(i=0; i<n; i++)
      d[i]=new node_id[n];
    for(i=0; i<n; i++)
      { d[i][i]=0;
        for(j=0; j<n; j++)
          { if (i!=j)
	      { if (g->HasEdge(i,j))
	          d[i][j]=1;
		else
		  d[i][j]=NULL_NODE;
	       }
	   }
       }

	for(k=0; k<n; k++)
     for(i=0; i<n; i++)
      for(j=0; j<n; j++)
	  { if ((long)d[i][k]+d[k][j] < d[i][j])
	      { d[i][j]=d[i][k]+d[k][j];
	      }
	  }

	return d;
}

static node_id** raw_charac(node_id** dist, int n){
	int i, j;
	node_id **d;
    d=new node_id *[n];
    for(i=0; i<n; i++){
      d[i]=new node_id[n];
	  for(int k=0; k<n; k++)
		  d[i][k] = 0;
	}
	for(i=0; i<n; i++){
		for(j=0; j<n; j++){
			if(dist[i][j]==NULL_NODE)
				d[i][n-1]++;
			else{
				if(dist[i][j]>0){
					d[i][dist[i][j]-1]++;
				}
			}
		}
	}
	return d;
}

static node_id** col_charac(node_id** dist, int n){
	int i, j;
	node_id **d;
    d=new node_id *[n];
    for(i=0; i<n; i++){
      d[i]=new node_id[n];
	  for(int k=0; k<n; k++)
		  d[i][k] = 0;
	}
	for(i=0; i<n; i++){
		for(j=0; j<n; j++){
			if(dist[j][i]==NULL_NODE)
				d[i][n-1]++;
			else{
				if(dist[j][i]>0){
					d[i][dist[j][i]-1]++;
				}
			}
		}
	}
	return d;
}



static bool compare_array(node_id *array1, node_id *array2, int n){
	int i;
	for(i=0; i<n; i++)
		if(array1[i] != array2[i])
			return false;
	return true;
}

static void initial_cls(node_id* cls1, node_id* cls2, node_id** raw_charac1,\
				node_id** raw_charac2, node_id** col_charac1, node_id** col_charac2,
				int n){
	int k, i;
	for(int i=0; i<n; i++){
		cls1[i]=0;
		cls2[i]=0;
	}
	for(k=0; k<n; k++){
		for(i=0; i<n; i++){
			if(cls1[i] < 1  &&  compare_array(raw_charac1[i], raw_charac1[k], n)
							&& compare_array(col_charac1[i], col_charac1[k], n)){
				cls1[i] = k+1;
			}

			if(cls2[i] < 1  &&  compare_array(raw_charac2[i], raw_charac1[k], n)
							&& compare_array(col_charac2[i], col_charac1[k], n)){
				cls2[i] = k+1;
			}

		}
	}

        for(int i=0; i<n; i++){
            delete [] raw_charac1[i];
            delete [] raw_charac2[i];
            delete [] col_charac1[i];
            delete [] col_charac2[i];
        }
        delete [] raw_charac1;
        delete [] raw_charac2;
        delete [] col_charac1;
        delete [] col_charac2;
}


