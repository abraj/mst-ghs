#ifndef NODE_H
#define NODE_H

#include "utils.h"

class Node {
  public:
	int id;			// node id
	int SN;			// node state;	#SLEEPING, FIND, FOUND
	int LN;			// node level
	long FN;		// fragment identity
	int RT;			// root node of the fragment
	map<int,edge_t> edges;	// edges adjacent to the node
	list<string> msgq;	// message queue

	int in_branch;		// inbound branch
	int best_edge;		// best candidate in whose direction we may find min-wt outgoing edge
	int test_edge;
	int best_wt;
	int find_count;

	void *g_ref_nmap;
	void *g_ref_mutex;
	void *g_ref_out;
	int flag_halt;

	Node();
	Node(void*, void*, void*);
	void reset(int id);
	void dump();

	void proc_wakeup();
	void proc_test();
	void proc_report();
	void proc_change_root();

	void recv_spontaneous_wakeup();
	int recv_connect(int j, int L);
	void recv_initiate(int j, int L, long F, int S, int R);
	int recv_test(int j, int L, long F);
	void recv_accept(int j);
	void recv_reject(int j);
	int recv_report(int j, int w);
	void recv_change_root();

	void send_connect(int j, int L);
	void send_initiate(int j, int L, long F, int S, int R);
	void send_test(int j, int L, long F);
	void send_accept(int j);
	void send_reject(int j);
	void send_report(int j, int w);
	void send_change_root(int j);

	void print_mst(int j);
	void recv_print_mst(int j);
	void send_print_mst(int j);
	string format_edge(int j);

	void send_msg(int n, string msg);
	void recv_msg();
	int process_msg(string msg);

	int min_adj_edge();
	int min_adj_edge(int s);
	int get_edge_id(int n);
	int get_target(int e);
	int get_smaller(int e);
	void add_edge(int n, int w);
	void free_edges();
	void free_queue();
	void echo(string msg);
};

#endif /* NODE_H */

