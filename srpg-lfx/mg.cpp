//
//  mg.cpp
//  srpg
//
//  Created by iD Student on 6/25/14.
//  Copyright (c) 2014 pk inc. All rights reserved.
//

#include "mg.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include "config.h"

using namespace std;

int max_x, max_y;

class Pair {
public:
	int y; int x;
	Pair() {}
	Pair(int y, int x) {
		this->y = y;
		this->x = x;
	}
};

bool valid(int y, int x) {return ((y >= 0 && y < max_y) && (x >= 0 && x < max_x));}

void normalize(vector< vector<int> > &map, int npass) {
	cout << "\nNormalizing...";
	for (int count = 0; count < npass; count++){
		
		for (int y = 0; y < max_y; y++) { // iterate over y
			for (int x = 0; x < max_x; x++) { // iterate over x
//				cout << "Smoothing.";
				if (map[y][x] != 0) { // if the slot is not 0
					
					for (int dx = -1; dx <= 1; dx++) { // iterate over the 8 slots arount the slot y,x
						for (int dy = -1; dy <= 1; dy++) { // ^
							
							if (dx == 0 && dy == 0) continue; // do not apply operation to self
					//		cout << "\n\tnpass " << count << " smoothing: [" << y + dy << ", " << x + dx << "]";
							if (!valid(y + dy, x + dx)) continue; // handles edge cases
							if (map[y][x] > map[y + dy][x + dx]) {
								map[y + dy][x + dx] += (float)(abs(map[y + dy][x + dx] - map[y][x])) * 0.3;
							
							}
						}
					}
				}
			}
		}
	}
}

void buildMap(const char *fn) {
//	std::string file = static_cast<const char *>(fn);
	std::string file = fn;
	srand((int)time(0));
	/*
	// GETTING INFO FROM USER
	cout << "Max X: ";
	cin >> max_x;
	cout << "Max Y: ";
	cin >> max_y;
	int nodes, npass;
	cout << "Nodes: ";
	cin >> nodes;
	cout << "Normalizing passes: ";
	cin >> npass;
	
	string usef;
	cout << "Use filler: [y/n]: ";
	cin >> usef;
	int algo; char tc;
	cout << "Algorithm: ([s]wept/[t]arget/[c]ross/[C]omposite): ";
	cin >> tc;
	if (tc == 't') {algo = 1;} else if (tc == 'c') {algo = 2;} else if (tc == 'C') {algo = 3;} else {algo = 0;}
	*/
	int algo = 0;
	max_x = MAPWIDTH;
	max_y = MAPHEIGHT;
	int nodes = (MAPWIDTH/10) + (MAPHEIGHT);
	string usef = "n";
	int npass = ((algo == 3) ? 4 : 10) + rand()%((algo == 3) ? 5 : 10);
	cout << "NPASS: " << npass;
	 // INITIALIZING MAP
	cout << "Initializing map...";
	vector< vector<int> > map;
	for (int y = 0; y < max_y; y++) {
		vector<int> tmp;
		for (int x = 0; x < max_x; x++) {
			tmp.push_back(0);
		}
		map.push_back(tmp);
	}
	cout << "\nAdding nodes...";
	for (int c = 0; c < nodes; c++) {
		map[rand()%max_y][rand()%max_x] = (rand()%200);
	}
	/*cout << "\nmap state:\n"; // debug
	 for (int y = 0; y < max_y; y++) {
	 for (int x = 0; x < max_x; x++) {
	 cout << map[y][x] << "|";
	 }
	 cout << "\n";
	 }*/
	normalize(map, npass);
	if (algo == 1) {
		vector<Pair> targets;
		int maxv = 0;
		
		for (int c = 0; c < npass; c++) {
			cout << "Normalizing...";
			targets.clear();
			for (int y = 0; y < max_y; y++) {
				for (int x = 0; x < max_x; x++) {
					if (map[y][x] > maxv) {maxv = map[y][x];}
				}
			}
			for (int y = 0; y < max_y; y++) {
				for (int x = 0; x < max_x; x++) {
					if (map[y][x] > maxv/2) {
						targets.push_back(Pair(y,x));
					}
				}
			}
			for (int cn = 0; cn < targets.size(); cn++) {
				Pair tmp = targets.at(cn);
				for (int dx = -1; dx <= 1; dx++) {
					for (int dy = -1; dy <= 1; dy++) { // ^
						if (dx == 0 && dy == 0) continue; // do not apply operatio to self
						if (!valid(tmp.y + dy, tmp.x + dx)) continue; // handles edge cases
						if (map[tmp.y][tmp.x] > map[tmp.y + dy][tmp.x + dx]) {
							map[tmp.y + dy][tmp.x + dx] += (float)(abs(map[tmp.y + dy][tmp.x + dx] - map[tmp.y][tmp.x])) * 0.5;
						}
					}
				}
			}
		}
	} else if (algo == 3) {
		bool ranf;
		cout << "\tnormalizing with composite algorithm\n";
		for (int c = 0; c < npass; c++) {
			bool rx = (rand()%2 == 0);
			bool ry = (rand()%2 == 0);
			ranf = false;
			for (int y = ((ry) ? 0 : max_y - 1); ((ry) ? (y < max_y) : (y >= 0) ); y += ((ry) ? 1 : -1)) {
				for (int x = ((rx) ? 0 : max_x - 1); ((rx) ? (x < max_x) : (x >= 0) ); x += ((rx) ? 1 : -1)) {
					for (int dx = -1; dx <= 1; dx++) { // iterate over the 8 slots arount the slot y,x
						for (int dy = -1; dy <= 1; dy++) { // ^
							if (dx == 0 && dy == 0) continue; // do not apply operatio to self
							if (!valid(y + dy, x + dx)) continue; // handles edge cases
							if (map[y][x] > map[y + dy][x + dx]) {
								map[y + dy][x + dx] += (float)(abs(map[y + dy][x + dx] - map[y][x])) * 0.3;
							}
							ranf = true;
						}
					}
				}
			}
			if (rx) {
				for (int y = 0; y < max_y; y++) {
					reverse(map[y].begin(), map[y].end());
				}
			}
			if (ry) {
				reverse(map.begin(), map.end());
			}
			cout << "\tOne pass complete. ry: " << ry << " rx: " << rx << "ranf: " << ranf << "\n";
		}
		
	}
	/*	cout << "\nmap state:\n"; // debug
	 for (int y = 0; y < max_y; y++) {
	 for (int x = 0; x < max_x; x++) {
	 cout << map[y][x] << "|";
	 }
	 cout << "\n";
	 }*/
	cout << "\nQuantizing...";
	vector<string> values;
	values.push_back(string("003"));
	values.push_back(string("004"));
	values.push_back(string("005"));
	values.push_back(string("002"));
	values.push_back(string("001"));
	values.push_back(string("000"));
	values.push_back(string("066"));
	values.push_back(string("000"));
	values.push_back(string("066"));
	values.push_back(string("065"));
	
	int min = 0, max = 0, range;
	for (int y = 0; y < max_y; y++) {
		for (int x = 0; x < max_x; x++) {
			if (map[y][x] < min) {
				min = map[y][x];
			}
			if (map[y][x] > max) {
				max = map[y][x];
			}
		}
	}
	range = min + max;
	for (int y = 0; y < max_y; y++) {
		for (int x = 0; x < max_x; x++) {
			map[y][x] += min;
		}
	}
	for (int y = 0; y < max_y; y++) {
		for (int x = 0; x < max_x; x++) {
			map[y][x] = floor(((float)map[y][x] / (float)range) * 10.0f);
		}
	}
	/*	cout << "\nmap state:\n"; // debug
	 for (int y = 0; y < max_y; y++) {
	 for (int x = 0; x < max_x; x++) {
	 cout << map[y][x] << "|";
	 }
	 cout << "\n";
	 }*/
	cout << "\nFinal pass...";
	for (int y = 0; y < max_y; y++) {
		for (int x = 0; x < max_x; x++) {
			if (map[y][x] > 9) {
				map[y][x] = 9;
			} else if (map[y][x] < 0) {
				map[y][x] = 0;
			}
		}
	}
	cout << "\nWriting to file...";
	ofstream of;
	of.open(file.c_str());
	for (int y = 0; y < max_y; y++) {
		for (int x = 0; x < max_x; x++) {
			of << map[y][x] << ((usef == "y") ? string("|") : string(" "));
		}
		if (y + 1 != max_y) of << '\n';
	}
	of.close();
	cout << "\nProcess complete.\n";
}