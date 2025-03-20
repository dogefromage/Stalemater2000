import torch
from position import Position

class Metrics():

    def __init__(self, dir_output):
        self.dir_output = dir_output

    def dataset_mae(self, estimates: torch.Tensor, positions: list[Position]):
        return ...