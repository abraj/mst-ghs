#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <map>
#include <list>
#include <vector>
#include <cmath>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <pthread.h>

#define INF 999
#define NIL -1
#define NODES_MAX 64
#define DEBUG_MODE 0
#define DEBUG_MODE2 1
#define OUTPUT 1

using namespace std;

enum node_state {
	DEFAULT1,
	SLEEPING,
	FIND,
	FOUND
};

enum edge_state {	
	DEFAULT2,
	BASIC, 
	BRANCH, 
	REJECTED
};

typedef struct {
	int SE;		// edge state;	#BASIC, BRANCH, REJECTED
	int W;		// edge weight
} edge_t;


void test_env();
void init_edge(edge_t *e);
void set_edge_wt(edge_t *e, int W);
vector<string> &split(const string &s, char delim, vector<string> &elems);
vector<string> split(const string &s, char delim);
void replace_all(string& str, const string& from, const string& to);
void trim(string&);
void nsort(string&);
void f_write(string);

#endif /* UTILS_H */

