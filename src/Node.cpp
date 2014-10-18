#include "Node.h"

Node::Node()
{
	reset(NIL);
}

Node::Node(void *ref_nmap, void *ref_mutex, void *ref_out)
{
	this->g_ref_nmap = ref_nmap;
	this->g_ref_mutex = ref_mutex;
	this->g_ref_out = ref_out;
	reset(NIL);
}

void Node::reset(int id)
{
	this->id = id;

	/* Mandatory initialization */
	SN = SLEEPING;
	flag_halt = 0;

	/* Optional initialization */
	LN = 0;
	FN = NIL;
	RT = NIL;
	best_edge = test_edge = in_branch = NIL;
	best_wt = NIL;
	find_count = NIL;

	/* reset edge list and message queue */
	free_edges();
	free_queue();
}

void Node::dump()
{
	map<int,edge_t>::iterator it;
	list<string>::iterator itc;

	cout << "=== [" << id << "] ===" << endl;
	cout << "node_state  : " << SN << " (1:SLEEPING, 2:FIND, 3:FOUND)" << endl;
	cout << "node_level  : " << LN << endl;
	cout << "fragment_id, root : " << FN << ", " << RT << endl;
	cout << best_edge << ", " << test_edge << ", " << in_branch << " (best_edge, test_edge, in_branch)" << endl;
	cout << best_wt << ", " << find_count << ", " << flag_halt << " (best_wt, find_count, flag_halt)" << endl;

	cout << "Edges: [state, weight] (State# 1:BASIC, 2:BRANCH, 3:REJECTED)" << endl;
	for (it=edges.begin(); it!=edges.end(); ++it) {
		int i = it->first;
		cout << "  " << i << ": " << edges[i].SE << ", " << edges[i].W << endl;
	}

	cout << "Message Queue:" << endl;
	for (itc = msgq.begin(); itc != msgq.end(); ++itc) {
	    cout << *itc << " -> ";
	}
	cout << endl;

	cout << "------------------------------------------" << endl;
}

/****************************************************/

void Node::recv_spontaneous_wakeup()
{
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] recv_spontaneous_wakeup()", this->id); echo(dmsg); }

	proc_wakeup();
}

void Node::proc_wakeup() 
{
	int m;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] proc_wakeup()", this->id); echo(dmsg); }

	m = min_adj_edge();
	if(DEBUG_MODE) { sprintf(dmsg, "[%d] min_adj_edge = %d", this->id, m); echo(dmsg); }
	
	edges[m].SE = BRANCH;
	LN = 0;
	SN = FOUND;
	find_count = 0;

	send_connect(m, LN);
}

int Node::recv_connect(int j, int L)
{
	int flag_defer = 0;
	int log, exp;
	long F;
	int root;
	char dmsg[100];

	if (SN == SLEEPING) proc_wakeup();

	if(DEBUG_MODE && (L < LN || edges[j].SE != BASIC)) { sprintf(dmsg, "[%d] recv_connect(%d, %d)", this->id, j, L); echo(dmsg); }
	
	if (L < LN) {
		edges[j].SE = BRANCH;
		send_initiate(j, LN, FN, SN, RT);
		if (SN == FIND) find_count++;
	}
	else if (edges[j].SE == BASIC) {
		flag_defer = 1;	// defer_connect(j, L)
	}
	else {
		log = (int)log10(NODES_MAX);
		exp = pow(10,(log+1));

		F = edges[j].W*exp*exp + j;
		root = get_smaller(j);
		send_initiate(j, LN + 1, F, FIND, root);
	}

	return flag_defer;
}

void Node::recv_initiate(int j, int L, long F, int S, int R)
{
	map<int,edge_t>::iterator it;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] recv_initiate(%d, %d, %ld, %d, %d)", this->id, j, L, F, S, R); echo(dmsg); }

	LN = L; 
	SN = S; 
	FN = F; 
	RT = R; 
	in_branch = j;

	if(DEBUG_MODE2) { sprintf(dmsg, "[%d] New_level: %d; Set_branch: --> %d",this->id, L, j); echo(dmsg);}

	best_edge = NIL; 
	best_wt = INF;

	for (it=edges.begin(); it!=edges.end(); ++it) {
		int i	= it->first;

		if(i != j && edges[i].SE == BRANCH) {
			send_initiate(i, L, F, S, R);
			if (S == FIND) find_count++;
		}
	}

	if (S == FIND) proc_test();
}

void Node::proc_test()
{
	int test_edge;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] proc_test()", this->id); echo(dmsg); }

	test_edge = min_adj_edge(BASIC);

	if (test_edge != NIL) {
		send_test(test_edge, LN, FN);
	}
	else {
		proc_report();
	}
}

int Node::recv_test(int j, int L, long F)
{
	int flag_defer = 0;
	char dmsg[100];

	if(DEBUG_MODE && L > LN) { sprintf(dmsg, "[%d] recv_test(%d, %d, %ld)", this->id, j, L, F); echo(dmsg); }

	if (SN == SLEEPING) proc_wakeup();

	if (L > LN) {
		flag_defer = 1;	// defer_test(j, L, F)
	}
	else if (F != FN) {
		send_accept(j);
	}
	else {
		if (edges[j].SE == BASIC) edges[j].SE = REJECTED;

		if (test_edge != j) send_reject(j);
		else proc_test();
	}

	return flag_defer;
}

void Node::recv_accept(int j)
{
	test_edge = NIL;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] recv_accept(%d)", this->id, j); echo(dmsg); }

	if (edges[j].W < best_wt) {
		best_edge = j;
		best_wt = edges[j].W;
	}

	proc_report();
}

void Node::recv_reject(int j)
{
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] recv_reject(%d)", this->id, j); echo(dmsg); }

	if (edges[j].SE == BASIC) edges[j].SE = REJECTED;

	proc_test();
}

void Node::proc_report()
{
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] proc_report()", this->id); echo(dmsg); }

	if (find_count == 0 && test_edge == NIL) {
		SN = FOUND;
		send_report(in_branch, best_wt);
	}
}

int Node::recv_report(int j, int w)
{
	int flag_defer = 0;
	char dmsg[100];

	if(DEBUG_MODE && (j != in_branch || SN != FIND)) { sprintf(dmsg, "[%d] recv_report(%d, %d)", this->id, j, w); echo(dmsg); }

	if (j != in_branch) {
		find_count--;
		if (w < best_wt) {
			best_wt = w;
			best_edge = j;
		}
		proc_report();
	}
	else if (SN == FIND) {
		flag_defer = 1;	// defer_report(j, w)
	}
	else if (w > best_wt) {
		proc_change_root();
	}
	else if (w == best_wt && best_wt == INF) {
		if(RT == id) {
			recv_print_mst(NIL);
		}
	}

	return flag_defer;
}

void Node::proc_change_root()
{
	char dmsg[100];

	if(best_edge == NIL) return;

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] proc_change_root()", this->id); echo(dmsg); }

	if (edges[best_edge].SE == BRANCH) {
		send_change_root(best_edge);
	}
	else {
		send_connect(best_edge, LN);
		edges[best_edge].SE = BRANCH;
	}
}

void Node::recv_change_root()
{
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] recv_change_root()", this->id); echo(dmsg); }

	proc_change_root();
}

/****************************************************/

void Node::print_mst(int j)
{
	char msg[100];

	sprintf(msg, "%s", format_edge(j).c_str());
	(*((stringstream *)g_ref_out)) << msg << endl;
}

void Node::recv_print_mst(int j)
{
	map<int, edge_t>::iterator it;

	for (it=edges.begin(); it!=edges.end(); ++it) {
		int i = it->first;

		if(i != j && edges[i].SE == BRANCH) {
			print_mst(i);
			send_print_mst(i);
		}
	}

	flag_halt = 1;
}

void Node::send_print_mst(int j)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	
	n = sprintf(buffer, "%s:%d", "PRINT", j);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

/****************************************************/

void Node::send_connect(int j, int L)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_connect(%d, %d)", this->id, j, L); echo(dmsg); }

	n = sprintf(buffer, "%s:%d:%d", "CONNECT", j, L);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

void Node::send_initiate(int j, int L, long F, int S, int R)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_initiate(%d, %d, %ld, %d, %d)", this->id, j, L, F, S, R); echo(dmsg); }
	
	n = sprintf(buffer, "%s:%d:%d:%ld:%d:%d", "INITIATE", j, L, F, S, R);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

void Node::send_test(int j, int L, long F)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_test(%d, %d, %ld)", this->id, j, L, F); echo(dmsg); }

	
	n = sprintf(buffer, "%s:%d:%d:%ld", "TEST", j, L, F);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

void Node::send_accept(int j)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_accept(%d)", this->id, j); echo(dmsg); }
	
	n = sprintf(buffer, "%s:%d", "ACCEPT", j);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

void Node::send_reject(int j)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_reject(%d)", this->id, j); echo(dmsg); }
	
	n = sprintf(buffer, "%s:%d", "REJECT", j);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

void Node::send_report(int j, int w)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_report(%d, %d)", this->id, j, w); echo(dmsg); }
	
	n = sprintf(buffer, "%s:%d:%d", "REPORT", j, w);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

void Node::send_change_root(int j)
{
	char buffer[100];
	int n;
	int target_node = get_target(j);
	string msg;
	char dmsg[100];

	if(DEBUG_MODE) { sprintf(dmsg, "[%d] send_change_root(%d)", this->id, j); echo(dmsg); }
	
	n = sprintf(buffer, "%s:%d", "CHANGE_ROOT", j);
	buffer[n] = '\0';
	msg = string(buffer);

	send_msg(target_node, msg);
}

/****************************************************/

void Node::send_msg(int n, string msg)
{
	map<int, Node> *g_nmap = (map<int, Node> *)g_ref_nmap;
	pthread_mutex_t *g_mutex = (pthread_mutex_t *)g_ref_mutex;

	/* mutex code : ADD */
	pthread_mutex_lock(g_mutex);
	(*g_nmap)[n].msgq.push_back(msg);
	pthread_mutex_unlock(g_mutex);
}

void Node::recv_msg()
{
	int flag_defer;
	string msg;
	list<string>::iterator it;
	pthread_mutex_t *g_mutex = (pthread_mutex_t *)g_ref_mutex;

	while(1) 
	{
		while(msgq.size() == 0 && flag_halt == 0) 
		{
			usleep(10*1000);
		}

		if(flag_halt != 0) {
			break;
		}

		it = msgq.begin();
		msg = *it;
		flag_defer = process_msg(msg);

		if(flag_defer == 1) {
			/* mutex code : DEFER */
			pthread_mutex_lock(g_mutex);
			msgq.pop_front();
			msgq.push_back(msg);
			pthread_mutex_unlock(g_mutex);
		}
		else {
			/* mutex code : REMOVE */
			pthread_mutex_lock(g_mutex);
			msgq.pop_front();
			pthread_mutex_unlock(g_mutex);
		}
	}
}

int Node::process_msg(string msg)
{
	vector<string>::iterator it;
	int flag_defer = 0;
	int a,b,c,d;
	long l;

	vector<string> tokens = split(msg, ':');
	it = tokens.begin();

	if((*it).compare("WAKEUP") == 0) {
		recv_spontaneous_wakeup();
	}
	else if((*it).compare("CONNECT") == 0) {
		it++; a = atoi((*it).c_str());
		it++; b = atoi((*it).c_str());
		flag_defer = recv_connect(a, b);
	}
	else if((*it).compare("INITIATE") == 0) {
		it++; a = atoi((*it).c_str());
		it++; b = atoi((*it).c_str());
		it++; l = atoi((*it).c_str());	// Since sizeof(int) = 4 = sizeof(long)
		it++; c = atoi((*it).c_str());
		it++; d = atoi((*it).c_str());
		recv_initiate(a, b, l, c, d);
	}
	else if((*it).compare("TEST") == 0) {
		it++; a = atoi((*it).c_str());
		it++; b = atoi((*it).c_str());
		it++; l = atoi((*it).c_str());	// Since sizeof(int) = 4 = sizeof(long)
		flag_defer = recv_test(a, b, l);
	}
	else if((*it).compare("ACCEPT") == 0) {
		it++; a = atoi((*it).c_str());
		recv_accept(a);
	}
	else if((*it).compare("REJECT") == 0) {
		it++; a = atoi((*it).c_str());
		recv_reject(a);
	}
	else if((*it).compare("REPORT") == 0) {
		it++; a = atoi((*it).c_str());
		it++; b = atoi((*it).c_str());
		flag_defer = recv_report(a, b);
	}
	else if((*it).compare("CHANGE_ROOT") == 0) {
		recv_change_root();
	}
	else if((*it).compare("PRINT") == 0) {
		it++; a = atoi((*it).c_str());
		recv_print_mst(a);
	}
	else if((*it).compare("DUMMY") == 0) {
		// Used for debugging
	}
	else {
		echo("ERROR! Invalid Message token.");
		exit(0);
	}

	return flag_defer;
}

/****************************************************/

// return the minimum-weight adjacent edge
int Node::min_adj_edge()
{
	map<int,edge_t>::iterator it;
	int min_wt = INF;
	int min_edge = NIL;

	for (it=edges.begin(); it!=edges.end(); ++it) {
		int i = it->first;
		if(edges[i].W < min_wt) {
			min_wt = edges[i].W;
			min_edge = i;
		}
	}

	return min_edge;
}

// return the minimum-weight adjacent edge in state s
int Node::min_adj_edge(int s)
{
	map<int,edge_t>::iterator it;
	int min_wt = INF;
	int min_edge = NIL;

	for (it=edges.begin(); it!=edges.end(); ++it) {
		int i = it->first;
		if(edges[i].W < min_wt && edges[i].SE == s) {
			min_wt = edges[i].W;
			min_edge = i;
		}
	}

	return min_edge;
}

int Node::get_edge_id(int n)
{
	int a, b;
	int log, exp;

	if(this->id < n)  {
		a = this->id;
		b = n;
	}
	else {
		a = n;
		b = this->id;
	}

	log = (int)log10(NODES_MAX);
	exp = pow(10,(log+1));
	
	return a*exp + b;
}

int Node::get_target(int e)
{
	int a, b;
	int log, exp;

	log = (int)log10(NODES_MAX);
	exp = pow(10,(log+1));
	
	a = e/exp;
	b = e%exp;

	if(a == this->id) {
		return b;
	}
	else if(b == this->id) {
		return a;
	}

	return NIL;
}

string Node::format_edge(int e)
{
	int a, b;
	int log, exp;
	char msg[100], fmt[50];

	log = (int)log10(NODES_MAX);
	exp = pow(10,(log+1));
	
	a = e/exp;
	b = e%exp;

	sprintf(fmt, " %%0%dd --> %%0%dd",(log+1),(log+1));
	sprintf(msg, fmt, a, b);

	return string(msg);
}

int Node::get_smaller(int e)
{
	int a, b;
	int log, exp;

	log = (int)log10(NODES_MAX);
	exp = pow(10,(log+1));
	
	a = e/exp;
	b = e%exp;

	if(a < b) {
		return a;
	}
	else {
		return b;
	}
}

void Node::add_edge(int n, int w)
{
	edge_t e;

	init_edge(&e);
	set_edge_wt(&e,w);
	edges[get_edge_id(n)] = e;
}

void Node::free_edges()
{
	map<int,edge_t>::iterator it;

	for (it=edges.begin(); it!=edges.end(); ++it) {
		int key = it->first;
		edges.erase(key);
	}
}

void Node::free_queue()
{
	int i,n;
	n = msgq.size();

	for (i=1; i<=n; i++) {
		msgq.pop_front();
	}
}

void Node::echo(string msg)
{
	pthread_mutex_t *g_mutex = (pthread_mutex_t *)g_ref_mutex;

	pthread_mutex_lock(g_mutex);
	cout << msg << endl;
	fflush(stdout);
	pthread_mutex_unlock(g_mutex);
}


