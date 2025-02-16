import io
import zstandard as zstd
import json
import re
import chess
import torch

def read_jsonl_zst(file_path):
  with open(file_path, 'rb') as file:
    decompressor = zstd.ZstdDecompressor()
    stream_reader = decompressor.stream_reader(file)
    stream = io.TextIOWrapper(stream_reader, encoding = "utf-8")
    for line in stream:
      yield json.loads(line)

def get_fen_and_score(dataset_point):
    fen = dataset_point['fen']
    deepest_eval = sorted(dataset_point['evals'], key=lambda x: x['depth'], reverse=True)[0]
    first_pv = deepest_eval['pvs'][0]

    side_to_move = re.search("^[\w\d/]+ (w|b)", fen).group(1)

    score = 0
    if 'mate' in first_pv:
        mate_value = first_pv['mate']
        if mate_value < 0:
            score = -10000 + mate_value
        else:
            score = 10000 - mate_value
    else:
        score = first_pv['cp']

    return fen, score, side_to_move
   
piece_to_index = {
    'P': 0, 'N': 1, 'B': 2, 'R': 3, 'Q': 4, 'K': 5,  # White pieces
    'p': 6, 'n': 7, 'b': 8, 'r': 9, 'q': 10, 'k': 11  # Black pieces
}

def fen_to_tensor(fen):
    board = chess.Board(fen)
    tensor = torch.zeros(768, dtype=torch.uint8)
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            index = piece_to_index[piece.symbol()]
            tensor[64 * index + square] = 1
    return tensor

dataset_path = "datasets/lichess_db_eval.jsonl.zst"
size = 1000000

dataset_boards = torch.zeros(size, 768)
dataset_black_to_move = torch.zeros(size, 1)
dataset_evals = torch.zeros(size, 1)

for i, point in enumerate(read_jsonl_zst(dataset_path)):
    if i >= size:
        break

    fen, score, side_to_move = get_fen_and_score(point)
    input_tensor = fen_to_tensor(fen)

    dataset_boards[i] = input_tensor
    dataset_black_to_move[i] = 1 if side_to_move == 'b' else 0
    dataset_evals[i] = score

    if i % 10000 == 0:
        print(f"{i} converted")

torch.save((dataset_boards, dataset_black_to_move, dataset_evals), f"dataset_{size}.pt")
