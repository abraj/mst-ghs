#include "utils.h"
#include <algorithm>

void test_env()
{
	if(sizeof(int) < 4) {
		printf("ERROR! sizeof(int) must be 4\n\n");
		fflush(stdout);
		exit(0);
	}
}

void init_edge(edge_t *e) 
{
	/* Mandatory initialization */
	e->SE = BASIC;

	/* Optional initialization */
	e->W = 0;
}

void set_edge_wt(edge_t *e, int W)
{
	e->W = W;
}

/*---------------------------------------*/

vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

/*---------------------------------------*/

void replace_all(string& str, const string& from, const string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

/*---------------------------------------*/

void trim(string &str)
{
	str.erase(str.find_last_not_of(" \n\r\t")+1);
}

void nsort(string &str)
{
	FILE *fp;
	string inp, out;
	char cmd[2000], rsp[2000];
	
	inp = str;
	replace_all(inp, "\r", "");
	replace_all(inp, "\n", "\\n");

	sprintf(cmd, "echo \"%s\" | sort", inp.c_str());
	fp = popen(cmd, "r");
	fscanf(fp, "%[^*]", rsp);
	pclose(fp);

	out = string(rsp);
	trim(out);
	replace_all(out, "01", " 1");
	replace_all(out, "02", " 2");
	replace_all(out, "03", " 3");
	replace_all(out, "04", " 4");
	replace_all(out, "05", " 5");
	replace_all(out, "06", " 6");
	replace_all(out, "07", " 7");
	replace_all(out, "08", " 8");
	replace_all(out, "09", " 9");
	
	str = out;
}

void f_write(string str)
{
	ofstream file;
	file.open("io/output.txt");
	file << str;
	file.close();
}


