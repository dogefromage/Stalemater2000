#pragma once
#include "BoardMakros.cpp"
#include "Board.cpp"
#include <string>
#include <vector>

using namespace std;

class Game
{
public:
	Board board = Board::Default();

	void Print()
	{
		board.Print();
	}
};