#pragma once
#include "Evaluation.h"

struct Position
{
	Score score = 0;
	int bestMove = 0;
	short searchID = 0;

	Position() {};

	Position(const short& _score, const int& _bestMove, const int& _searchID)
	{
		score = _score;
		bestMove = _bestMove;
		searchID = _searchID;
	}
};