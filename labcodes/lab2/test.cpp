#include <cstdio>
#include <cstring>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

struct Block {
	int id, begin, size;
	Block(int id, int begin, int size) : id(id), begin(begin), size(size) {
	}
};

int f[20000];
vector<Block> v;
int n = 16384;

Block alloc_t(int id, int size) {
	int s = 1;
	while (s < size) s = s * 2;
	size = s;
	for(int i = 1; i < n; i += size) {
		bool flag = true;
		for(int j = i; j < i + size; j ++)
			if (f[j] == 1) flag = false;
		if (flag) {
			for(int j = i; j < i + size; j++) {
				f[j] = 1;
			}
			return Block(id, i, size);
		}
	}
	return Block(0, 0, 0);
}

void free_t(int begin, int size) {
	for(int i = begin; i < begin + size; i++) {
		f[i] = 0;
	}
}


void check_size() {
	int cnt = 0;
	for(int i = 1; i <= n; i++) cnt += 1 - f[i];
	printf("assert(nr_free_pages() == %d);\n", cnt);
}

void check() {
	for(int i = 1; i < v.size(); i++) {
		printf("assert(p%d == p%d + (%d));\n", v[i].id, v[0].id, v[i].begin - v[0].begin);
	}
}


int main() {
	 srand(unsigned(time(NULL)));
	 int remain = n;
	 for(int i = 1; i <= n; i++) f[i] = 0;
	 for(int i = 0; i < 20; i++) {
		 int t = rand() % 7;
		 if (remain == 0) {
			 t = 0;
		 }
		 if (t == 0) { // free
			 if (v.size() == 0) {
				 t = 1;
			 }
			 else {
				 random_shuffle(v.begin(), v.end());
				 Block b = v[v.size() - 1];
				 v.pop_back();
				 free_t(b.begin, b.size);
				 remain += b.size;
				 printf("free_pages(p%d, %d);\n", b.id, b.size);
			 }
		 }
		 if (t > 0) { // alloc
			 while (1) {
				 int size = rand() % max((remain), 1) + 1;
				 //printf("alloc %d %d\n", remain, size);
				 Block b = alloc_t(i, size);
				 if (b.size != 0) {
					 printf("struct Page *p%d = alloc_pages(%d);\n", b.id, b.size);
					 v.push_back(b);
					 remain -= b.size;
					 break;
				 }
				 else {
					 printf("assert(alloc_pages(%d) == NULL);\n", size);
				 }
			 }
		 }
		 if (v.size() > 1 && i % 10 == 0) {
			 check();
			 check_size();
		 }
	 }
	 for(int i = 0; i < v.size(); i++) {
		 free_t(v[i].begin, v[i].size);
		 printf("free_pages(p%d, %d);\n", v[i].id, v[i].size);
	 }
	 check_size();
}

