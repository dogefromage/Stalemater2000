from datetime import datetime
import os
from pathlib import Path

import torch

from dataset import DataloaderCreator
from train import train
from model import NNUE

def run_experiment(dir_output, dir_data, params):

    # path_train_set = dir_data / "head_50k.jsonl.zst"
    # train_count = 50_000
    path_train_set = dir_data / "tail_10m.jsonl.zst"
    train_count = 10_000_000
    path_validation_set = dir_data / "head_50k.jsonl.zst"
    valid_size = 50_000

    train_loader_creator = DataloaderCreator(path_train_set, train_count, params['batch_size'], params['num_workers'])
    validation_loader_creator = DataloaderCreator(path_validation_set, valid_size, params['batch_size'], params['num_workers'])
    model = NNUE()

    train(dir_output, params, train_loader_creator, validation_loader_creator, model)

def get_datetime_string():
    return datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

if __name__ == '__main__':

    dir_base = Path("/home/seb/git/Stalemater2000/nnue")
    assert os.path.isdir(dir_base)

    dir_data = dir_base / 'data'

    dir_name = f"exp_{get_datetime_string()}"
    dir_output = dir_base / 'output' / dir_name
    os.makedirs(dir_output, exist_ok=True)

    print("Open tensorboard with this:")
    print(f"tensorboard --logdir {dir_base / 'output'}")

    params = {
        'learning_rate': 1e-3,
        'weight_decay': 1e-5,

        'batch_size': 256,
        'num_epochs': 100,
        'num_training_slices': 10,
        'num_workers': 4,
    }

    run_experiment(dir_output, dir_data, params)