#pragma once
#include "PuzzleState.h"
using namespace std;
enum MoveType { down, left, right, up, nullMove }; 
//legal moves that can be made to obtain a certain path. NullMove means not move was made to get to a state
class PuzzleMove
{
public:
		PuzzleMove() { }
		PuzzleMove(PuzzleState s, PuzzleMove* p, MoveType m) : state(s), parent(p), move(m)
		{ }
		const PuzzleState & getState() const
		{
				return state;
		}
		PuzzleMove*  getParent()const
		{
				return parent;
		}
		MoveType getMoveName()const
		{
				return move;
		}
private:
		PuzzleState state; //keeps track of the state
		PuzzleMove* parent; //keeps track of the state's parent
		MoveType move; //keeps track of the move made to obtain the state
};