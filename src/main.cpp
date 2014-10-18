#include "Node.h"
#include "fstream"

void dump(map<int, Node> nodes);
void* handler(void *arg);

int main() 
{
	stringstream out;
	string output;
	map<int, Node> nodes;
	map<int, pthread_t> threads;
	map<int, pthread_t>::iterator it;

	pthread_mutex_t gmutex;
	pthread_mutex_init(&gmutex, NULL);

	Node n((void*)&nodes, (void*)&gmutex, (void*)&out);
	pthread_t th;

	system("clear");
	test_env();

	/*------------------------------*/
/*	n.reset(3);
	n.add_edge(2,3);
	n.add_edge(1,5);
	nodes[n.id] = n;
	threads[n.id] = th; 
*/

	ifstream infile("./io/input.txt");
	int nodeCount, nodeData;
	if (!infile.is_open()) {
		cout<<"Error in opening file"<<endl;
		return 1;
	}
	infile >> nodeCount;
	for (int i = 0; i < nodeCount; i++) {
		n.reset(i+1);
		for (int j = 0; j< nodeCount; j++) {
			infile >> nodeData;
			if (nodeData > 0)

			  n.add_edge(j+1, nodeData);
		}
		nodes[n.id] = n;
		threads[n.id] = th;
	}
	/*------------------------------*/

	/* create threads */
	for (it=threads.begin(); it!=threads.end(); ++it) {
		int i = it->first;
		pthread_create (&threads[i], NULL, &handler, (void*)&nodes[i]);
	}

	n.send_msg(1, "WAKEUP");

	/* join threads */
	for (it=threads.begin(); it!=threads.end(); ++it) {
		int i = it->first;
		pthread_join(threads[i], NULL);
	}

	output = out.str();
	trim(output);
	nsort(output);

	f_write(output);
	if(OUTPUT) cout << output << endl;

	pthread_mutex_destroy(&gmutex);

	return 0;
}

void* handler(void *arg)
{
	Node *n = (Node *)arg;
	n->recv_msg();
	return NULL;
}

// USAGE: dump(nodes);
void dump(map<int, Node> nodes)
{
	map<int, Node>::iterator it;

	cout << "======:: NODES ::======" << endl;
	for (it = nodes.begin(); it != nodes.end(); ++it) {
		int i = it->first;
		nodes[i].dump();
	}
}


