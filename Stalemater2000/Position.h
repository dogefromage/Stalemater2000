#pragma once
#include "Evaluation.h"

struct Position
{
	Score score;
	//short depth;

	Position() {}

	Position(const short& _score/*, const short& _depth*/)
	{
		score = _score;
		//depth = _depth;
	}
};