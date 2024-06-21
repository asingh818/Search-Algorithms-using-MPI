#include <mpi.h>
#include <vector>
#include <iostream>
#include <string>
#include <list>    //used for storing solution
#include <queue>   //needed for priority queue
#include <unordered_set>  //used for storing pointers to puzzle moves in open and close sets
#include <functional>   //for representing hash-function, key-comparison for searching hash-table, 
#include "PuzzleState.h"
#include "PuzzleMove.h"
#include <iomanip> 
#include "myTimer.h"
using namespace std;
namespace myHash {
	template <typename KeyType>
	class hash
	{
	public:
		size_t  operator()(const KeyType & k) const;
	};
	template<>
	class hash<PuzzleMove*> 
	{
	public:
		unsigned int operator()(const PuzzleMove* key) const //get hashvalue of certain PuzzleMove
		{
			size_t hashVal = 0;
			string perm = key->getState().getTilePermutation();
			for (char ch : perm)
				hashVal = hashVal * 37 + ch;
			return hashVal;
		}
	};
}

struct myKeyComparison : public std::binary_function<PuzzleMove*, PuzzleMove*, bool> //compare hashvalues in the hashtable
{
	bool operator()(const PuzzleMove* lhs, const PuzzleMove* rhs) const
	{
		return lhs->getState().getTilePermutation() == rhs->getState().getTilePermutation();
	}
};

struct mycomparison : public std::binary_function<PuzzleMove*, PuzzleMove*, bool> //compare hashvalues in the hashtable
{
	bool operator()(const PuzzleMove* lhs, const PuzzleMove* rhs) const
	{
		return lhs->getState().getEvaluation() > rhs->getState().getEvaluation();
	}
};

PuzzleMove* intersection(unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison> &closedPuzzleMoves, int rank);
bool find_solution(PuzzleState, PuzzleState, list<PuzzleMove> &);
bool bidirectional_solution(PuzzleState, PuzzleState, list<PuzzleMove> &, int);
void print_solution(list<PuzzleMove> &, int);
int main()
{
	int comm_sz;
	int my_rank;
	MPI_Init(0, 0);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz); //set number of ranks in comm_sz
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); //set rank in my_rank

	int numSize, size, moveMade;
	char data;
	PuzzleState currentState;
	PuzzleMove currentMove;
	if (my_rank == 0)
	{
		cout << "Enter number of rows and columns(should be greater then 1 ): ";
		cin >> numSize;
		while (!cin || numSize < 2) //if invalid input or the rows/columns is less then 2, input again
		{
			cout << "Invalid input! Please enter again" << endl;
			cin.clear();
			cin.ignore();
			cout << "Enter number of rows and columns(should be greater then 1 ): ";
			cin >> numSize;
		}
		PuzzleState start_state(numSize, numSize);
		PuzzleState goal_state(numSize, numSize);
		cout << "Enter Start State row by row(0 for the empty spot): " << endl;
		cin >> start_state;
		while (!start_state.valid()) //if invalid state, input again
		{
			cout << "Invalid input! Please enter again" << endl;
			cin.clear();
			cin.ignore();
			cout << "Enter Start State row by row(0 for the empty spot): " << endl;
			cin >> start_state;
		}
		cout << "Enter Goal State row by row(0 for the empty spot): " << endl; 
		cin >> goal_state;
		while (!start_state.valid()) //if invalid state, input again
		{
			cout << "Invalid input! Please enter again" << endl;
			cin.clear();
			cin.ignore();
			cout << "Enter Goal State row by row(0 for the empty spot): " << endl;
			cin >> goal_state;
		}
		//send numSize to every other rank
		MPI_Send(&numSize, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		MPI_Send(&numSize, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
		MPI_Send(&numSize, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
		int size = numSize * numSize;
		vector<char> startState = start_state.serialization(); //serialize the start state
		vector<char> goalState = goal_state.serialization(); //serialize the goal state
		for (unsigned int j = 0; j < startState.size(); j++)
		{
			data = startState[j];
			//send each variable in the vector one by one to rank 1 and 2
			MPI_Send(&data, sizeof(data), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
			MPI_Send(&data, sizeof(data), MPI_CHAR, 2, 0, MPI_COMM_WORLD);
		}
		for (unsigned int j = 0; j < goalState.size(); j++)
		{
			data = goalState[j];
			//send each variable in the vector one by one to rank 1 and 2
			MPI_Send(&data, sizeof(data), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
			MPI_Send(&data, sizeof(data), MPI_CHAR, 2, 0, MPI_COMM_WORLD);
		}
	}
	else if (my_rank == 1)
	{
		myTimer t;
		list<PuzzleMove> solution;
		MPI_Recv(&numSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Receive numSize from Rank 0
		vector<char> startState(5+(numSize*numSize)); //Plus 5 because of the 5 datatypes in each state
		vector<char> goalState(5+(numSize*numSize)); //Plus 5 because of the 5 datatypes in each state
		for (unsigned int i = 0; i < startState.size(); i++)
		{
			MPI_Recv(&data, sizeof(data), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
			//Receive each value in rank 0's serilization vector for start state
			startState[i] = data; //Set the values in startState vector
		}

		for (unsigned int i = 0; i < goalState.size(); i++)
		{
			MPI_Recv(&data, sizeof(data), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//Receive each value in rank 0's serilization vector for goal state
			goalState[i] = data; //Set the values in goalState vector
		}
		PuzzleState start_state;
		PuzzleState goal_state;
		start_state.deserialization(startState); //Reconstruct the start state 
		goal_state.deserialization(goalState); //Reconstruct the goal state 

		t.start(); //start timer
		if (start_state.heuristic(goal_state) <= 5) //If heuristic is less then equal to 5, do not need to do a Bidirectional search
		{
			find_solution(start_state, goal_state, solution);
		}
		else //the higher the heuristic, the more moves it would need to take to get to the goal so a Bidirectional search is appropriate
		{
			bidirectional_solution(start_state, goal_state, solution, my_rank);
		}
		t.stop(); //stop timer
		cout << "Elapsed time (s): " << t.time() << endl; //Print the amount of time it took to complete the search

		size = solution.size();
		MPI_Send(&size, 1, MPI_INT, 3, 0, MPI_COMM_WORLD); //Send the size of rank 1's solution list to rank 3
		for (list<PuzzleMove>::const_iterator itr = solution.begin(); itr != solution.end(); ++itr)
		{
			//Go through each PuzzleMove in the solution list and send the state & move to Rank 3
			currentState = itr->getState();
			vector<char> state = currentState.serialization();
			for (unsigned int j = 0; j < state.size(); j++)
			{
				data = state[j];
				MPI_Send(&data, 1, MPI_CHAR, 3, 0, MPI_COMM_WORLD);
			}
			moveMade = itr->getMoveName();
			MPI_Send(&moveMade, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
		}
	}
	else if (my_rank == 2)
	{
		list<PuzzleMove> solution;
		MPI_Recv(&numSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //receive numSize from Rank 0
		vector<char> startState(5+(numSize*numSize)); //Plus 5 because of the 5 datatypes in each state
		vector<char> goalState(5 + (numSize*numSize)); //Plus 5 because of the 5 datatypes in each state
		for (unsigned int j = 0; j < startState.size(); j++) 
		{
			MPI_Recv(&data, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//Receive each value in Rank 0's serialization vector for goal state
			startState[j] = data; //set the values in start vector
		}
		for (unsigned int j = 0; j < goalState.size(); j++)
		{
			MPI_Recv(&data, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
			//Receive each value in Rank 0's serialization vector for goal state
			goalState[j] = data; //set the values in goalState vector
		}
		PuzzleState start_state;
		PuzzleState goal_state;
		start_state.deserialization(startState); //Reconstruct the start state
		goal_state.deserialization(goalState); //Reconstruct the goal state
		if (start_state.heuristic(goal_state) > 5) //If heuristic is less then equal to 5, do not need to do a bidirectional search
		{
			bidirectional_solution(goal_state, start_state, solution, my_rank);
		}
		size = solution.size();
		MPI_Send(&size, 1, MPI_INT, 3, 0, MPI_COMM_WORLD); //Send the size of rank 2's solution list to rank 3
		for (list<PuzzleMove>::const_iterator itr = solution.begin(); itr != solution.end(); ++itr)
		{
			//Go through each PuzzleMove in the solution list and send the State and Move to Rank 3
			currentState = itr->getState();
			vector<char> temp1 = currentState.serialization();
			for (unsigned int i = 0; i < temp1.size(); i++)
			{
				data = temp1[i];
				MPI_Send(&data, 1, MPI_CHAR, 3, 0, MPI_COMM_WORLD);
			}
			moveMade = itr->getMoveName(); //Store index of moveName for sending
			MPI_Send(&moveMade, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
		}
	}
	else if (my_rank == 3)
	{
		list<PuzzleMove> solution;
		MoveType movetype;
		vector<MoveType> movement{ down, MoveType::left, MoveType::right, up, nullMove };
		MPI_Recv(&numSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //receive numSize from Rank 0
		//receiving Rank 1's solution list
		MPI_Recv(&size, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //receive size from Rank 1
		vector<char> second_pi_states(5+(numSize*numSize)); //plus 5 because of the 5 datatypes in each state
		for (int i = 0; i < size; i++)
		{
			//Reconstruct the PuzzleMoves in Rank 1's solution list
			//Note: PuzzleMove* parent is set to null as it is not required when printing the solution
			for (unsigned int j = 0; j < second_pi_states.size(); j++)
			{
				MPI_Recv(&data, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				second_pi_states[j] = data;
			}
			MPI_Recv(&moveMade, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			PuzzleState pathState;
			pathState.deserialization(second_pi_states);
			movetype = movement.at(moveMade);
			solution.push_back(PuzzleMove(pathState, NULL, movetype));
		}
		if (solution.size() == 1) //if Rank 1's solution list size is 1, the goal state equals the start state
		{
			cout << "Goal State equals Start State " << endl;
		}
		else
		{
			print_solution(solution, 1); //Print Rank 1's solution list
		}
		solution.clear(); //clear list for Rank 2's solution list
		//receiving Rank 2's solution list
		MPI_Recv(&size, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //receive size from Rank 2
		vector<char> third_pi_states(5+(numSize*numSize)); //plus 5 because of the 5 datatypes in each state
		for (int i = 0; i < size; i++)
		{
			//reconstruct the PuzzleMoves in Rank 2's solution list
			//Note: PuzzleMove* parent is set to null as it is not required when printing the solution
			for (unsigned int j = 0; j < third_pi_states.size(); j++)
			{
				MPI_Recv(&data, 1, MPI_CHAR, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				third_pi_states[j] = data;
			}
			PuzzleState pathState;
			pathState.deserialization(third_pi_states);
			MPI_Recv(&moveMade, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //recieved index of the move made to obtain the state
			movetype = movement.at(moveMade); //Figure out what move was made to get to the received state
			solution.push_back(PuzzleMove(pathState, NULL, movetype));
		}
		print_solution(solution, 2); //Print Rank 2's solution list
	}
	MPI_Finalize();
	return 0;
}

bool isMember(PuzzleMove *p, const unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison>  &s) 
{
	return (s.find(p) != s.end());
}

void print_solution(list<PuzzleMove> &solution, int rank)
{
	if (rank == 1)
	{
		vector<string> movement{ "down", "left", "right", "up" };
		if (solution.size() == 0) //Since rank 1 will do a search regardless, if its list is empty, no path was found
		{
			cout << "Did not find a solution.\n";
		}
		else
		{
			cout << "\nSolution:\n\n";
			cout << "\nRank 1:\n\n";
			for (list<PuzzleMove>::const_iterator itr = solution.begin(); itr != solution.end(); ++itr)
			{
				if (itr->getMoveName() != nullMove) //if move is nullMove, it is the startstate so we do not need to print the move
				{
					cout << movement[itr->getMoveName()] << endl << endl;
				}
				PuzzleState currentState = itr->getState();
				cout << currentState << endl;
			}
		}
	}
	else if (rank == 2) // Rank 2's list is from goal to start so we need to reverse it
	{
		vector<string> movement{ "up", "right", "left", "down" }; //moves in reverse as the list was reversed
		if (solution.size() != 0) //Since rank 2 is not guaranteed to run a search, we would not have to do anything if the list is empty
		{
			cout << "\nRank 2:\n\n";
			for (list<PuzzleMove>::const_iterator itr = solution.begin(); itr != solution.end(); ++itr)
			{
				if (itr == solution.begin()) //if begin, it's the intersection so we just need the move from intersect to the next state
				{
					cout << movement[itr->getMoveName()] << endl << endl;
				}
				else
				{
					PuzzleState currentState = itr->getState();
					cout << currentState << endl; //print state first as the list moves are reversed
					if (itr->getMoveName() != nullMove)
					{
						cout << movement[itr->getMoveName()] << endl << endl;
					}
				}
			}
		}
	}
}

PuzzleMove* intersection(unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison> &openPuzzleMoves, int rank)
{
	PuzzleMove* intersect = nullptr; //by default, the interection is nullprt
	bool inter;
	int list_size, tileSize;
	char data;
	if (rank == 1)
	{
		list_size = openPuzzleMoves.size();
		MPI_Send(&list_size, 1, MPI_INT, 2, 0, MPI_COMM_WORLD); //send size of openlist to rank 2
		unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison>::iterator itr;
		for (itr = openPuzzleMoves.begin(); itr != openPuzzleMoves.end(); ++itr) //iterate through the open list
		{
			PuzzleState state = (*itr)->getState();
			tileSize = state.size();
			MPI_Send(&tileSize, 1, MPI_INT, 2, 0, MPI_COMM_WORLD); //send Rank 2 the size of state
			vector<char> info = state.serialization();
			for (int j = 0; j < (5+tileSize); j++)
			{
				data = info[j];
				MPI_Send(&data, 1, MPI_CHAR, 2, 0, MPI_COMM_WORLD); //send Rank 2 the serialized state bit by bit
			}
			//Rank 1 waits for Rank 2 to let it know if the state sent is an intersection
			MPI_Recv(&inter, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //receives boolean that indicates if intersection exists
			if (inter) //if intersection was found
			{
				intersect = *itr; //set the intersection variable to said intersect
				break; //once the intersection was found, break out of for loop
			}
		}
	}
	if (rank == 2)
	{
		MPI_Recv(&list_size, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //recieve Rank 1's open list size
		for (int i = 0; i < list_size; i++) 
		{
			MPI_Recv(&tileSize, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
			vector<char> rank1_State(5+tileSize);
			for (unsigned int j = 0; j < rank1_State.size(); j++)
			{
				MPI_Recv(&data, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				rank1_State[j] = data;
			}
			PuzzleState state;
			state.deserialization(rank1_State); //reconstruct state from Rank 1's open list
			PuzzleMove* member = new PuzzleMove(state, NULL, nullMove); //make a temporary puzzlemove pointer to look for intersection
			if (isMember(member, openPuzzleMoves)) //if state is in rank 2's open list, an intersection exists
			{
				inter = true; 
				MPI_Send(&inter, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); //let Rank 1 know an intersection was found
				intersect = *(openPuzzleMoves.find(member)); //set intersection to the PuzzleMove with said state
				delete member; //delete the temporary PuzzleMove pointer
				member = nullptr; //set temporary PuzzleMove pointer to nullptr
				break; //once the intersection was found, break out of for loop
			}
			else
			{
				inter = false; 
				MPI_Send(&inter, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); //let Rank 1 know an intersection was not found
				delete member; //delete the temporary PuzzleMove pointer
				member = nullptr; // set temporary PuzzleMove pointer to nullptr
			}
		}
	}
	return intersect; //return the intersection whether it is nullptr or an actually PuzzleMove pointer
}
bool find_solution(PuzzleState start, PuzzleState goal, list<PuzzleMove> & solution) //sequential search for shorter problems
{
	PuzzleState currentState;
	PuzzleMove* parentMovePtr;
	PuzzleState childState;
	int currentState_f_eval;
	int currentState_h;
	int currentState_g;
	long totalStates = 2; //count start and final states
	priority_queue<PuzzleMove*, vector<PuzzleMove*>, mycomparison> openPuzzleMoves_pq;  //fast access for choosing min val of eval function 
	unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison> openPuzzleMoves;
	unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison> closedPuzzleMoves;
	PuzzleMove* currentPuzzleMovePtr;  //used for pointing to a Move_node
	PuzzleMove* currentMovePtr;  //used for pointing to a Move_node while in open priority queue

	start.setEvaluation(start.heuristic(goal));
	currentPuzzleMovePtr = new PuzzleMove(start, nullptr, nullMove);  //create search node for rest of program
	openPuzzleMoves.insert(currentPuzzleMovePtr);  //initially put pointer to node in open list and...
	openPuzzleMoves_pq.push(currentPuzzleMovePtr);  //put pointer to it  in the priority queue
	unsigned counter = 0;
	while (!openPuzzleMoves_pq.empty())
	{
		currentMovePtr = openPuzzleMoves_pq.top();
		openPuzzleMoves_pq.pop();
		closedPuzzleMoves.insert(currentMovePtr);
		openPuzzleMoves.erase(currentMovePtr);
		currentState = currentMovePtr->getState();
		currentState_f_eval = currentState.getEvaluation();
		currentState_h = currentState.heuristic(goal);
		currentState_g = currentState_f_eval - currentState_h;
		if (currentState == goal) //If goal has been found, backtrack to find the path
		{
			solution.push_front(*currentMovePtr);
			parentMovePtr = currentMovePtr->getParent();
			while (parentMovePtr != nullptr) //backtrack until we find a state who's parent is null, the inital state
			{
				currentMovePtr = parentMovePtr;
				solution.push_front(*currentMovePtr);
				parentMovePtr = currentMovePtr->getParent();
			}
			return true;
		}
		else
		{
			PuzzleMove* newCurrentMovePtr; //Create temporary ptr
			if (currentState.canMoveDown())
			{
				childState = currentState.moveBlankDown(); //Create a child that had the blank move down
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::down); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
			if (currentState.canMoveLeft())
			{
				childState = currentState.moveBlankLeft(); //Create a child that had the blank move left
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::left); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
			if (currentState.canMoveUp())
			{
				childState = currentState.moveBlankUp(); //Create a child that had the blank move up
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::up); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
			if (currentState.canMoveRight())
			{
				childState = currentState.moveBlankRight(); //Create a child that had the blank move right
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::right); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
		}
	}
	return false;
}
bool bidirectional_solution(PuzzleState start, PuzzleState goal, list<PuzzleMove> & solution, int rank)
{
	PuzzleState currentState;
	int move = 0;
	PuzzleMove*  parentMovePtr;
	PuzzleState childState;
	int currentState_f_eval;
	int currentState_h;
	int currentState_g;
	long totalStates = 2; //count start and final states
	priority_queue<PuzzleMove*, vector<PuzzleMove*>, mycomparison> openPuzzleMoves_pq;  //fast access for choosing min val of eval function
	unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison> openPuzzleMoves; //fast access for finding duplicate states
	unordered_set<PuzzleMove*, myHash::hash<PuzzleMove*>, myKeyComparison> closedPuzzleMoves; //fast access for finding duplicate states
	PuzzleMove* currentPuzzleMovePtr;  //used for pointing to a Move_node
	PuzzleMove* currentMovePtr;  //used for pointing to a Move_node while in open priority queue
	PuzzleMove* intersect;

	start.setEvaluation(start.heuristic(goal));
	currentPuzzleMovePtr = new PuzzleMove(start, nullptr, nullMove);  //create search node for rest of program
	openPuzzleMoves.insert(currentPuzzleMovePtr);  //initially put pointer to node in open list and...
	openPuzzleMoves_pq.push(currentPuzzleMovePtr);  //put pointer to it  in the priority queue
	unsigned counter = 0;
	while (!openPuzzleMoves_pq.empty())
	{
		currentMovePtr = openPuzzleMoves_pq.top(); //PuzzleMove with the smallest heuristic
		closedPuzzleMoves.insert(currentMovePtr);
		openPuzzleMoves_pq.pop();
		openPuzzleMoves.erase(currentMovePtr);
		currentState = currentMovePtr->getState();
		currentState_f_eval = currentState.getEvaluation();
		currentState_h = currentState.heuristic(goal);
		currentState_g = currentState_f_eval - currentState_h;
		if (++move % 2000 == 0) //Check for intersection every 2000 moves
		{
			intersect = intersection(openPuzzleMoves, rank);
			if (intersect != nullptr) //If intersection exists, backtrack to find the path
			{
				if (rank == 1)
				{
					solution.push_front(*intersect);
				}
				else if (rank == 2)
				{
					solution.push_back(*intersect); //push_back because rank 2's list will be reversed
				}
				parentMovePtr = intersect->getParent();
				while (parentMovePtr != nullptr) //keep backtracking until there is a state who's parent is null
				{
					intersect = parentMovePtr;
					if (rank == 1)
					{
						solution.push_front(*intersect);
					}
					else if (rank == 2)
					{
						solution.push_back(*intersect);
					}
					parentMovePtr = intersect->getParent();
				}
				return true;
			}
			else
			{
				delete intersect; //free up the memory if the intersection does not exist
			}
		}
		else
		{
			PuzzleMove* newCurrentMovePtr; //Create temporary ptr
			if (currentState.canMoveDown())
			{
				childState = currentState.moveBlankDown(); //Create a child that had the blank move down
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::down); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else 
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
			if (currentState.canMoveLeft())
			{
				childState = currentState.moveBlankLeft(); //Create a child that had the blank move left
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::left); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else 
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
			if (currentState.canMoveUp())
			{
				childState = currentState.moveBlankUp(); //Create a child that had the blank move up
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal));
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::up); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl;
					//Print totalStates every 1000 moves to check if the search is running
				}
				else 
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
			if (currentState.canMoveRight())
			{
				childState = currentState.moveBlankRight(); //Create a child that had the blank move right
				childState.setEvaluation((currentState_g + 1) + childState.heuristic(goal)); 
				//Set the evaluation based on how many moves said state is from initial + the heuristic of the state
				newCurrentMovePtr = new PuzzleMove(childState, currentMovePtr, MoveType::right); //Set temporary ptr
				if (!isMember(newCurrentMovePtr, openPuzzleMoves) && !isMember(newCurrentMovePtr, closedPuzzleMoves)) //If not in open or closed list
				{
					openPuzzleMoves.insert(newCurrentMovePtr);
					openPuzzleMoves_pq.push(newCurrentMovePtr);
					if ((++totalStates) % 1000 == 0) cout << "Total States = " << totalStates << endl; 
					//Print totalStates every 1000 moves to check if the search is running
				}
				else 
				{
					delete newCurrentMovePtr; //If not a member, delete the temporary ptr
				}
			}
		}
	}
	return false;
}