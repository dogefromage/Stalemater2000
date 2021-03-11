#pragma once
#include "Evaluation.h"

struct Position
{
	short searchID = 0;

	Position() {};

	Position(const short& _score, const short& _searchID)
	{
		searchID = _searchID;
	}
};