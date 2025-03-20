import chess
import torch

piece_to_index = {
    'P': 0, 'R': 1, 'N': 2, 'B': 3, 'Q': 4, 'K': 5,  # White pieces
    'p': 6, 'r': 7, 'n': 8, 'b': 9, 'q': 10, 'k': 11  # Black pieces
}

def board_to_tensor(board):
    tensor = torch.zeros(768, dtype=torch.uint8)
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            index = piece_to_index[piece.symbol()]
            tensor[64 * index + square] = 1
    return tensor

class Position():

    def __init__(self, fen, fixed_eval, side_to_move):
        self.fen = fen
        board = chess.Board(fen)
        self.inputs = board_to_tensor(board)
        self.fixed_eval = fixed_eval
        self.side_to_move = side_to_move