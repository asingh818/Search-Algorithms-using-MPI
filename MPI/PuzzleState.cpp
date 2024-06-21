#include "PuzzleState.h"
PuzzleState PuzzleState::NullState(0, 0);
void swap(char & x, char & y) //function used to swap two tiles in a state
{
	int temp = x;
	x = y;
	y = temp;
}

bool PuzzleState::operator==(const PuzzleState & rhs) const 
{
	if (rows != rhs.rows || cols != rhs.cols)
		return false;
	return tiles == rhs.tiles;
}
bool PuzzleState::operator!=(const PuzzleState & rhs) const
{
	return !(*this == rhs);
}
bool PuzzleState::operator<=(const PuzzleState & rhs) const
{
	return evaluation <= rhs.evaluation;
}

const PuzzleState & PuzzleState::operator=(const PuzzleState & rhs)
{
	if (this != &rhs)
	{
		tiles = rhs.tiles;
		rows = rhs.rows;
		cols = rhs.cols;
		evaluation = rhs.evaluation;
		blank_position_row = rhs.blank_position_row;
		blank_position_col = rhs.blank_position_col;
	}
	return *this;
}

const string PuzzleState::getTilePermutation() const
{
	string tilePermutation;
	for (char ch : tiles)
	{
		tilePermutation += ch;
	}
	return tilePermutation; //get tilePermutation to which will be used to get a hashvalue
}
bool PuzzleState::canMoveUp()
{
	return blank_position_row != 0; //if blank_position_row is 0, it is in the first row and cannot move up
}
bool PuzzleState::canMoveDown()
{
	return blank_position_row != (rows - 1); //if blank_position_row is rows - 1, it is in the last row and cannot move down
}

bool PuzzleState::canMoveLeft()
{
	return blank_position_col != 0; //if blank_position_col is 0, it is in the first column and cannot move left
}

bool PuzzleState::canMoveRight()
{
	return blank_position_col != (cols - 1); //if blank_position_row is cols - 1, it is in the last column and cannot move right
}

PuzzleState PuzzleState::moveBlankUp()
{
	PuzzleState S(*this);
	swap(S.tiles[blank_position_row*cols + blank_position_col], //swap the empty spot with the tile above it
		S.tiles[(blank_position_row - 1)*cols + blank_position_col]);
	S.blank_position_row = blank_position_row - 1; //update blank_position_row
	return S;
}

PuzzleState PuzzleState::moveBlankDown()
{
	PuzzleState S(*this);
	swap(S.tiles[blank_position_row*cols + blank_position_col], //swap the empty spot with the tile below it
		S.tiles[(blank_position_row + 1)*cols + blank_position_col]);
	S.blank_position_row = blank_position_row + 1; //update blank_position_row
	return S;
}

PuzzleState PuzzleState::moveBlankLeft()
{
	PuzzleState S(*this);
	swap(S.tiles[blank_position_row*cols + blank_position_col], //swap the empty spot with the tile to the left of it
		S.tiles[blank_position_row*cols + blank_position_col - 1]);
	S.blank_position_col = blank_position_col - 1; //update blank_position_col
	return S;
}
PuzzleState PuzzleState::moveBlankRight()
{
	PuzzleState S(*this);
	swap(S.tiles[blank_position_row*cols + blank_position_col], //swap the empty spot with the tile to the right of it
		S.tiles[blank_position_row*cols + blank_position_col + 1]);
	S.blank_position_col = blank_position_col + 1; //update blank_position_col
	return S;
}
bool PuzzleState::valid() //count number of blank spots. If not 1 exactly, not a valid state
{
	int count = 0;
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			if (tiles[i*cols + j] == '0')
			{
				count++;
				if (count > 1)
				{
					return false;
				}
			}
		}
	}
	if (count == 0)
	{
		return false;
	}
	return true;
}
void PuzzleState::read(istream & in)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			in >> tiles[i*cols + j];
			if (tiles[i*cols + j] == '0') //if the read in vaiable is '0', it is the empty spot
			{
				blank_position_row = i;
				blank_position_col = j;
			}
		}
	}
}
int PuzzleState::heuristic(const PuzzleState & goal)  //heuristic function returns value and modifies state object's h
{
	int sum = 0; //manhattan distance
	char tileNum;
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			tileNum = tiles[i*cols + j];
			if (tileNum != '0') //not the empty space in this state tile arrangement
			{
				bool found = false;
				for (int k = 0; k < rows && !found; k++)
				{
					for (int p = 0; p < cols && !found; p++)
					{
						if (goal.tiles[k*cols + p] == tileNum)
						{
							found = true;
							sum += (abs(i - k) + abs(j - p)); 
							//calculate how many moves it would take to get to the goal ignoring other tiles in between
						}
					}
				}
			}
		}
	}
	return sum; //return the manhattan distance for every tile not including the empty space
}

vector<char> PuzzleState::serialization()
{
	vector<char> serializeVector(5 + tiles.size()); // The serializeVector stores all the information of the state
	//Plus 5 since the size of the vector would be the tiles vector and the 5 datatypes
	//1: rows
	//2: cols
	//3: blank_position_row
	//4: blank_position_col
	//5: evaluation
	serializeVector[0] = '0' + this->rows; //convert int to char
	serializeVector[1] = '0' + this->cols; //convert int to char
	serializeVector[2] = '0' + this->blank_position_row; //convert int to char
	serializeVector[3] = '0' + this->blank_position_col; //convert int to char
	serializeVector[4] = '0' + this->evaluation; //convert int to char
	for (unsigned int i = 0; i < tiles.size(); i++)
	{
		serializeVector[5 + i] = tiles.at(i); //start from position 5
	}
	return serializeVector;
}

PuzzleState PuzzleState::deserialization(vector<char> &serializeVector)
{
	this->rows = serializeVector[0] - '0'; //convert char to int
	this->cols = serializeVector[1] - '0'; //convert char to int
	this->blank_position_row = serializeVector[2] - '0'; //convert char to int
	this->blank_position_col = serializeVector[3] - '0'; //convert char to int
	this->evaluation = serializeVector[4] - '0'; //convert char to int
	this->tiles.reserve(this->rows*this->cols); //reserve the size of the tile which should be rows * columns
	for (int i = 0; i < (this->rows*this->cols); i++)
	{
		this->tiles.push_back(serializeVector[5 + i]); //ignore the first 5 in the vector
	}
	return *this;
}

void PuzzleState::print(ostream & out)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			out << tiles[i*cols + j] << " ";
		}
		out << endl;
	}
}
istream & operator >> (istream &  in, PuzzleState & rhs)
{
	rhs.read(in);
	return in;
}

ostream & operator<<(ostream & out, PuzzleState  & rhs)
{
	rhs.print(out);
	return out;
}