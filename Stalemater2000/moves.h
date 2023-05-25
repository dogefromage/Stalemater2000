#pragma once
#include "boardlabels.h"
#include <string>
#include <optional>
#include <cassert>
#include <cstdint>
#include <vector>

struct LanMove {
	std::uint16_t move;
	/*
	* MSB [  4b  |  6b  |  6b  ] LSB
	*     [ prom |  to  | from ]
	*/
	LanMove(int from, int to, MovePromotions promotion) {
		assert(from >= 0 && from < 64);
		assert(to >= 0 && to < 64);
		move = from | (to << 6);
		move |= (std::uint16_t)promotion << 12;
	}
	int from() const {
		return move & 0x3f;
	}
	int to() const {
		return (move >> 6) & 0x3f;
	}
	MovePromotions promotion() const {
		return (MovePromotions)((move >> 12) & 0xf);
	}

	std::string toString() const;
	static std::string squareIndexToString(int index);
	static std::optional<LanMove> parseLanMove(const std::string& moveString);
	static std::optional<int> parseSquareIndex(const std::string& squareName);
};

// use maybe for assertions
// constexpr std::uint32_t GEN_MOVE_FLAG = 0x80000000;

struct GenMove {
	/*
	* MSB [ 8b |  4b  | 4b |  4b  |  6b  |  6b  ] LSB
	*     [    | type | bb | prom |  to  | from ]
	*/
	std::uint32_t move;

	GenMove(int from, int to, MovePromotions promotion, BitBoards bitBoard, MoveTypes type) {
		std::uint32_t bb = (std::uint32_t)bitBoard;
		assert(from >= 0 && from < 64);
		assert(to >= 0 && to < 64);
		assert(bb >= 0 && bb < 12);
		move = from | (to << 6);
		move |= (std::uint32_t)promotion << 12;
		move |= (std::uint32_t)bb << 16;
		move |= (std::uint32_t)type << 20;
	}

	int from() const {
		return move & 0x3f;
	}
	int to() const {
		return (move >> 6) & 0x3f;
	}
	MovePromotions promotion() const {
		return (MovePromotions)((move >> 12) & 0xf);
	}
	BitBoards bitboard() const {
		return (BitBoards)((move >> 16) & 0xf);
	}
	MoveTypes movetype() const {
		return (MoveTypes)((move >> 20) & 0xf);
	}

    // matches promotion, to, from
	bool matchesLanMove(const LanMove& lanm) const {
		return (move & 0xffff) == lanm.move;
	}

	std::string toString() const;
};

typedef std::vector<GenMove> MoveList;