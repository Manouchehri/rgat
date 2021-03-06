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
Monsterous class that handles the bulk of graph management 
*/

#pragma once
#include "stdafx.h"
#include "node_data.h"
#include "edge_data.h"
#include "graph_display_data.h"
#include "traceMisc.h"
#include "OSspecific.h"

//max length to display in diff summary
#define MAX_DIFF_PATH_LENGTH 50
#define ANIMATION_ENDED -1
#define ANIMATION_WIDTH 8

struct EXTERNCALLDATA {
	NODEPAIR edgeIdx;
	unsigned int nodeIdx;
	ARGLIST argList;
	MEM_ADDRESS callerAddr = 0;
	string externPath;
	bool drawFloating = false;
};

class thread_graph_data
{
	GRAPH_DISPLAY_DATA *mainnodesdata = 0;
	GRAPH_DISPLAY_DATA *mainlinedata = 0;

	EDGEMAP edgeDict; //node id pairs to edge data
	EDGELIST edgeList; //order of edge execution

	unsigned int lastAnimatedBB = 0;
	unsigned int firstAnimatedBB = 0;
	vector<node_data> nodeList; //node id to node data

	
	PROCESS_DATA* piddata;
	int baseMod = -1;
	HANDLE disassemblyMutex;

	//these are the edges/nodes that are brightend in the animation
	map <NODEPAIR, edge_data *> activeEdgeMap;
	//<index, final (still active) node>
	map <unsigned int, bool> activeNodeMap;

	HANDLE edMutex = CreateMutex(NULL, FALSE, NULL);
	HANDLE nodeLMutex = CreateMutex(NULL, FALSE, NULL);

	bool advance_sequence(bool);
	bool decrease_sequence();

	bool loadEdgeDict(ifstream *file);
	bool loadExterns(ifstream *file);
	bool loadNodes(ifstream *file, map <MEM_ADDRESS, INSLIST> *disassembly);
	bool loadStats(ifstream *file);
	bool loadAnimationData(ifstream *file);
	bool loadCallSequence(ifstream *file);

	//which BB we are pointing to in the sequence list
	unsigned long sequenceIndex = 0;
	//which instruction we are pointing to in the BB
	unsigned long blockInstruction = 0;
	bool newanim = true;
	unsigned int last_anim_start = 0;
	unsigned int last_anim_stop = 0;

	void *trace_reader;
	
	//updated with backlog input/processing each second for display
	//dunno if ulong reads are atomic, not vital for this application
	//adding accessor functions for future threadsafe acesss though
	pair<unsigned long, unsigned long> backlogInOut = make_pair(0, 0);
	

public:
	thread_graph_data(PROCESS_DATA* processdata, unsigned int threadID);
	~thread_graph_data();

	void display_active(bool showNodes, bool showEdges);
	void display_static(bool showNodes, bool showEdges);

	int render_edge(NODEPAIR ePair, GRAPH_DISPLAY_DATA *edgedata, map<int, ALLEGRO_COLOR> *lineColours,
		ALLEGRO_COLOR *forceColour = 0, bool preview = false);
	
	bool edge_exists(NODEPAIR edge, edge_data **edged);
	void add_edge(edge_data e, NODEPAIR edge);
	void insert_node(int targVertID, node_data node); 
	void extend_faded_edges();
	void assign_modpath(PROCESS_DATA *);
	GRAPH_DISPLAY_DATA *get_mainlines() { return mainlinedata; }
	GRAPH_DISPLAY_DATA *get_mainnodes() { return mainnodesdata; }
	GRAPH_DISPLAY_DATA *get_previewnodes() { return previewnodes; }
	GRAPH_DISPLAY_DATA *get_activelines() { return animlinedata; }
	GRAPH_DISPLAY_DATA *get_activenodes() { return animnodesdata; }
	void render_new_edges(bool doResize, map<int, ALLEGRO_COLOR> *lineColoursArr);

	unsigned int fill_extern_log(ALLEGRO_TEXTLOG *textlog, unsigned int logSize);

	bool serialise(ofstream *file);
	bool unserialise(ifstream *file, map <MEM_ADDRESS, INSLIST> *disassembly);
	//string get_mod_name(map <int, string> *modpaths);
	bool basic = false;

	//these are called a lot. make sure as efficient as possible
	inline edge_data *get_edge(NODEPAIR edge);
	edge_data * get_edge(int edgeindex);
	inline node_data *get_node(unsigned int index);

	bool node_exists(unsigned int idx) { if (nodeList.size() > idx) return true; return false; }
	unsigned int get_num_nodes() { return nodeList.size();}
	unsigned int get_num_edges() { return edgeDict.size();}

	void start_edgeD_iteration(EDGEMAP::iterator *edgeit, EDGEMAP::iterator *edgeEnd);
	void stop_edgeD_iteration();

	void start_edgeL_iteration(EDGELIST::iterator *edgeIt, EDGELIST::iterator *edgeEnd);
	void stop_edgeL_iteration();

	//i feel like this misses the point, idea is to iterate safely
	EDGELIST *edgeLptr() { return &edgeList; } 

	void animate_latest(float fadeRate);

	INS_DATA* get_last_instruction(unsigned long sequenceId);
	string get_node_sym(unsigned int idx, PROCESS_DATA* piddata);

	//if block at targetSequence called something, this highlights it. if sendArg true, adds floating latest arg to animation
	void brighten_externs(unsigned long targetSequence, bool updateArgs);
	void transferNewLiveCalls(map <int, vector<EXTTEXT>> *externFloatingText, PROCESS_DATA* piddata);

	void reset_mainlines();
	unsigned int derive_anim_node();
	void performStep(int stepSize, bool skipLoop);
	unsigned int updateAnimation(unsigned int updateSize, bool animationMode, bool skipLoop);
	VCOORD *get_active_node_coord();
	void set_active_node(unsigned int idx) {	
		if (nodeList.size() <= idx) return;
		obtainMutex(animationListsMutex, 1032);
		latest_active_node = get_node(idx);
		latest_active_node_coord = latest_active_node->vcoord;
		dropMutex(animationListsMutex);
	}
	void update_animation_render(float fadeRate);
	void reset_animation();
	void darken_animation(float alphaDelta);

	//during live animation the sequence list is ahead of the rendering
	//returns the last rendered sequenceIndex we managed to animate
	int brighten_BBs();

	void set_edge_alpha(NODEPAIR eIdx, GRAPH_DISPLAY_DATA *edgesdata, float alpha);
	void set_node_alpha(unsigned int nIdx, GRAPH_DISPLAY_DATA *nodesdata, float alpha);
	void emptyArgQueue();
	vector <string> loggedCalls;

	VCOORD latest_active_node_coord;
	node_data *latest_active_node = 0;

	unsigned int tid = 0;
	unsigned int pid = 0;
	bool active = true;
	bool terminated = false;

	ALLEGRO_BITMAP *previewBMP = NULL;
	//sym/arg strings that are to be made floating
	std::queue<EXTERNCALLDATA> floatingExternsQueue;

	HANDLE animationListsMutex = CreateMutex(NULL, FALSE, NULL);
	//ordered list of externs externs called by each node
	map<unsigned int, EDGELIST> externCallSequence;

	//list of external calls used for listing possible highlights
	vector<int> externList; 
	string modPath;

	HANDLE funcQueueMutex = CreateMutex(NULL, FALSE, NULL);
	//number of times each extern called, used for tracking which arg to display
	map <unsigned int, unsigned long> callCounter;

	//keep track of graph dimensions
	int maxA = 0;
	int maxB = 0;
	unsigned int bigBMod = 0;
	long zoomLevel = 0;

	unsigned long maxWeight = 0;
	unsigned long vertResizeIndex = 0;

	MULTIPLIERS *m_scalefactors = NULL;
	MULTIPLIERS *p_scalefactors = NULL;


	bool needVBOReload_main = true;
	GLuint graphVBOs[4] = { 0,0,0,0 };

	HANDLE graphwritingMutex = CreateMutex(NULL, FALSE, NULL);
	
	bool isGraphBusy() { 
		bool busy = (WaitForSingleObject(graphwritingMutex, 0) == WAIT_TIMEOUT); 
		if (!busy)
			ReleaseMutex(graphwritingMutex); 
		return busy;
	}

	void setGraphBusy(bool set) { 
		if (set) { 
			DWORD res = WaitForSingleObject(graphwritingMutex, 1000); 
			if (res == WAIT_TIMEOUT)
				cerr << "[rgat]Timeout waiting for release of graph "<< tid <<endl;
			assert(res != WAIT_TIMEOUT);
		}
		else ReleaseMutex(graphwritingMutex); 
	}

	bool VBOsGenned = false;
	//node+edge col+pos
	bool needVBOReload_preview = true;
	bool previewNeedsResize = false;
	GLuint previewVBOs[4] = { 0,0,0,0 };
	GRAPH_DISPLAY_DATA *previewnodes = 0;
	GRAPH_DISPLAY_DATA *previewlines = 0;

	bool dirtyHeatmap = false;
	bool needVBOReload_heatmap = true;
	//lowest/highest numbers of edge iterations
	pair<unsigned long,unsigned long> heatExtremes;
	GLuint heatmapEdgeVBO[1] = { 0 };
	GRAPH_DISPLAY_DATA *heatmaplines = 0;

	bool dirtyConditional = false;
	bool needVBOReload_conditional = true;
	//number of taken, not taken conditionals
	pair<unsigned long, unsigned long> condCounts;
	GLuint conditionalVBOs[2] = { 0 };
	GRAPH_DISPLAY_DATA *conditionallines = 0;
	GRAPH_DISPLAY_DATA *conditionalnodes = 0;

	//todo: make private, add inserter
	vector <pair<MEM_ADDRESS,unsigned int>> bbsequence; //block address, number of instructions
	vector <BLOCK_IDENTIFIER> mutationSequence; //blockID

	//<which loop this is, how many iterations of this block>
	//todo: make private, add inserter
	vector <pair<unsigned int, unsigned long>> loopStateList;

	//record how many times each block in loop has been animated
	vector<unsigned long> animLoopProgress;
	//index into sequence where start of loop is
	unsigned long animLoopStartIdx = 0;
	//current progress animating loop
	unsigned long animLoopIndex = 0;
	//total number of individual loops
	unsigned int loopCounter = 0;

	//position out of all the instructions instrumented
	unsigned long animInstructionIndex = 0;
	unsigned long totalInstructions = 0;

	unsigned int loopsPlayed = 0;
	unsigned long loopIteration = 0;
	unsigned long targetIterations = 0;

	bool needVBOReload_active = true;
	//two sets of VBOs for graph so we can display one
	//while the other is being written
	int lastVBO = 2;
	GLuint activeVBOs[4] = { 0,0,0,0 };

	//active areas + inactive areas
	GRAPH_DISPLAY_DATA *animnodesdata = 0;
	GRAPH_DISPLAY_DATA *animlinedata = 0;

	void display_highlight_lines(vector<node_data *> *nodeList, ALLEGRO_COLOR *colour, int lengthModifier);

	unsigned long traceBufferSize = 0;
	void *getReader() { return trace_reader;}
	void setReader(void *newReader) { trace_reader = newReader;}

	void setBacklogIn(unsigned long in) { backlogInOut.first = in; }
	void setBacklogOut(unsigned long out) { backlogInOut.second = out; }
	unsigned long getBacklogIn() { return backlogInOut.first; }
	unsigned long getBacklogOut() { return backlogInOut.second; }
	unsigned long get_backlog_total();
	bool terminationFlag = false;
};

