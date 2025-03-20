import re
import subprocess
import time
import torch
from dataset import fen_to_tensor, get_fen_and_score, read_jsonl_zst
from model import NNUE, load_checkpoint
import pandas as pd

# create multiple evaluations of some positions after various steps and compare to baseline

def get_random_fens(size):
    raw_data_path = "data/lichess_db_eval.jsonl.zst"

    testing_data = []
    
    for i, point in enumerate(read_jsonl_zst(raw_data_path)):
        if i >= size:
            break
        testing_data.append(get_fen_and_score(point))

    df = pd.DataFrame(testing_data, columns=['fen', 'baseline', 'side'])
    return df


with torch.no_grad():
    testing_nnue = NNUE()
    testing_nnue.eval()
    model_path = "nnue/nnue_ep_0120.pt"
    load_checkpoint(model_path, testing_nnue)

def evaluate_using_model(fen, side):
    boards = fen_to_tensor(fen).unsqueeze(0).float()
    black_to_move = torch.zeros(1, 1, dtype=torch.bool)
    black_to_move[0][0] = 1 if side == 'b' else 0
    with torch.no_grad():
        estimate = testing_nnue(boards, black_to_move)
    score = estimate.item()
    return score

def evaluate_using_engine(fen):
    process = subprocess.Popen(
        "bin/stalemater",
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    process.stdin.write(f"position fen {fen}\nd\n")
    process.stdin.flush()

    while True:
        line = process.stdout.readline()
        if not line:
            break

        eval_match = re.search("Eval: (-?\\d+)", line)
        if eval_match:
            evaluation = eval_match.group(1)
            print(f"EVAL={evaluation}")
            process.stdin.close()
            process.kill()
            return int(evaluation)
        
    raise Exception("Could not extract evaluation from engine")


if __name__ == '__main__':
    print("Generating fens")
    df = get_random_fens(20)

    print("Evaluating using engine")
    evaluations = [ evaluate_using_engine(fen) for _, (fen, *_) in df.iterrows() ]
    df = df.assign(evaluation = evaluations)

    print("Evaluating using model")
    estimates = [ evaluate_using_model(fen, side) for _, (fen, _, side, *_) in df.iterrows() ]
    df = df.assign(estimate = estimates)

    print(df)


