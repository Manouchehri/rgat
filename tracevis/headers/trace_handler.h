/*
Copyright 2016 Nia Catlin

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
Header for the thread that builds a graph for each trace
*/

#pragma once
#include "traceStructs.h"
#include "node_data.h"
#include "edge_data.h"
#include "thread_graph_data.h"
#include "thread_trace_reader.h"
#include "GUIStructs.h"
#include "timeline.h"
#include "base_thread.h"

#define NO_LOOP 0
#define BUILDING_LOOP 1
#define LOOP_PROGRESS 2

struct TAG {
	//come from trace
	MEM_ADDRESS blockaddr;
	BLOCK_IDENTIFIER blockID;
	unsigned long insCount;
	//used internally
	int jumpModifier;
	BB_DATA* foundExtern;
};
struct FAILEDARGS {
	int caller;
	int externid;
	MEM_ADDRESS targaddr;
};

class thread_trace_handler : public base_thread
{
public:
	thread_trace_handler(thread_graph_data *graph, unsigned int thisPID, unsigned int thisTID)
		:base_thread(thisPID, thisTID)
	{
		thisgraph = graph;
	}

	PROCESS_DATA *piddata;
	timeline *timelinebuilder;
	thread_trace_reader *reader;
	bool basicMode = false;
	void set_max_arg_storage(unsigned int maxargs) { arg_storage_capacity = maxargs; }
	bool *saveFlag;

private:
	void main_loop();
	thread_graph_data *graph;

	//important state variables!
	unsigned int lastVertID = 0; //the vert that led to new instruction
	unsigned int targVertID = 0; //new vert we are creating

	char lastRIPType = FIRST_IN_THREAD;
	vector<pair<MEM_ADDRESS, int>> callStack;

	thread_graph_data *thisgraph;
	//keep track of which a,b coords are occupied
	map<int, map<int, bool>> usedCoords;

	void handle_arg(char * entry, size_t entrySize);
	void process_new_args();
	bool run_external(MEM_ADDRESS targaddr, unsigned long repeats, NODEPAIR *resultPair);

	
	void runBB(TAG *tag, int startIndex, int repeats);
	void positionVert(int *pa, int *pb, int *pbMod, MEM_ADDRESS address);
	void updateStats(int a, int b, unsigned int bMod);
	void insert_edge(edge_data e, NODEPAIR edgepair);
	bool set_target_instruction(INS_DATA *instruction);
	void handle_new_instruction(INS_DATA *instruction, BLOCK_IDENTIFIER blockID);
	void handle_existing_instruction(INS_DATA *instruction);
	bool get_extern_at_address(MEM_ADDRESS address, BB_DATA ** BB, int attempts);
	bool find_internal_at_address(MEM_ADDRESS address, int attempts);
	void increaseWeight(edge_data *edge, unsigned long executions);
	void handle_tag(TAG *thistag, unsigned long repeats);
	void update_conditional_state(MEM_ADDRESS nextAddress);
	int find_containing_module(MEM_ADDRESS address);
	void dump_loop();


	MEM_ADDRESS pendingFunc = 0;
	MEM_ADDRESS pendingRet = 0;
	ARGLIST pendingArgs;
	//   function 	      caller		
	map<MEM_ADDRESS, map <MEM_ADDRESS, vector<ARGLIST>>> pendingcallargs;
	vector<FAILEDARGS> repeatArgAttempts;


	bool afterReturn = false;
	unsigned long loopCount = 0;
	unsigned int firstLoopVert = 0;
	int loopState = NO_LOOP;
	//tag address, mod type
	vector<TAG> loopCache;
	NODEPAIR repeatStart;
	NODEPAIR repeatEnd;
	unsigned int arg_storage_capacity = 100;
	
};