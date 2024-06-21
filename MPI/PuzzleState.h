/*
* This class describes what a PuzzleState is.
* The blank_position is for decreasing the time
* to check what tile operator is applicable
*/
#pragma once
#include <iostream>
#include <vector>
using namespace std;

class PuzzleState
{
public:
	PuzzleState() { }  // default constructor

	PuzzleState(int n, int m)  //constructor
	{
		tiles.resize(n*m); rows = n; cols = m;
	}
	~PuzzleState() { }   //destructor
	PuzzleState(const PuzzleState & rhs)   // copy constructor
		: rows(rhs.rows), cols(rhs.cols),
		tiles(rhs.tiles),
		evaluation(rhs.evaluation),
		blank_position_row(rhs.blank_position_row),
		blank_position_col(rhs.blank_position_col)
	{ }
	const PuzzleState & operator=(const PuzzleState & rhs); //assignment op
	bool operator==(const PuzzleState & rhs) const;  //compare tile arrangements
	bool operator!=(const PuzzleState & rhs) const;  //compare tile arrangements
	bool operator<=(const PuzzleState &) const; //compare two PuzzleState's evaluation functions f=g+h
	void setEvaluation(int g_plus_h_value) { evaluation = g_plus_h_value; };
	int getEvaluation() const { return evaluation; };
	int heuristic(const PuzzleState & goal);
	const string getTilePermutation() const;
	int size() { return tiles.size(); }
	vector<char> serialization();
	PuzzleState deserialization(vector<char> &tiles);
	bool valid();
	bool canMoveUp();
	bool canMoveDown();
	bool canMoveLeft();
	bool canMoveRight();
	PuzzleState moveBlankUp();
	PuzzleState moveBlankDown();
	PuzzleState moveBlankLeft();
	PuzzleState moveBlankRight();
	void read(istream & in);
	void print(ostream & out);
	static PuzzleState  NullState;
private:
	int rows;
	int cols;
	int evaluation;  //evaluation function f = g + h for PuzzleState object 
	vector<char> tiles;
	int blank_position_row;
	int blank_position_col;

};
istream & operator >> (istream &  in, PuzzleState & rhs);
ostream & operator<<(ostream & out, PuzzleState & rhs);