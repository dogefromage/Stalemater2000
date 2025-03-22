import os
from torch import nn
import torch

# class SCReLU(nn.Module):
#     def __init__(self):
#         super().__init__()

#     def forward(self, x):
#         return torch.clamp(x, min=0, max=1) ** 2

class LeakySCReLU(nn.Module):
    def __init__(self, l=0.01):
        super().__init__()
        self.l = l

    def forward(self, x):
        x_low = x * self.l
        x_high = self.l * (x - 1) + 1
        y = x**2
        y = torch.where(x < 0, x_low, y)
        y = torch.where(x > 1, x_high, y)

        return y

input_size = 768
hl_size = 1024
# output_scale = 400

num_output_buckets = 8

def get_occupancy(x: torch.tensor):
    occupancy, _ = x.reshape(-1, 12, 8, 8).max(dim=1)
    return occupancy

def get_output_bucket(x: torch.tensor):
    occupancy = get_occupancy(x)
    number_of_pieces = occupancy.reshape(-1, 64).sum(dim=1)
    div = 31 / num_output_buckets
    output_bucket = (number_of_pieces - 2) / div
    output_bucket = output_bucket.to(dtype=torch.int64)
    
    assert output_bucket.min(dim=-1)[0].item() >= 0
    # some positions have more pieces than normal chessboard, just clamp them and we're safe
    output_bucket = output_bucket.clamp(max=num_output_buckets - 1)

    return output_bucket
    

class NNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.accumulation_layer = nn.Linear(input_size, hl_size, bias=True)
        self.activation = LeakySCReLU()
        self.output_layer = nn.Linear(2 * hl_size, num_output_buckets, bias=True)

    def forward(self, x, black_to_move):

        output_bucket = get_output_bucket(x)

        # flip
        x_grouped = x.reshape(-1, 2, 6, 8, 8)
        x_flipped = torch.flip(x_grouped, [1, 3])
        x_flipped = x_flipped.reshape(-1, input_size)

        acc_white = self.accumulation_layer(x)
        acc_black = self.accumulation_layer(x_flipped)

        acc_normal = torch.cat([ acc_white, acc_black ], dim=1)
        acc_flipped = torch.cat([acc_black, acc_white], dim=1)

        # swap accumulators such that stm is first and nstm is second
        acc = torch.where(black_to_move, acc_flipped, acc_normal)
        acc = self.activation(acc)

        stm_eval_buckets = self.output_layer(acc)

        stm_eval = stm_eval_buckets.gather(1, output_bucket.unsqueeze(1))
        
        # mirror evaluation
        fixed_eval = torch.where(black_to_move, -stm_eval, stm_eval)

        return fixed_eval

def save_checkpoint(model, optimizer, epoch, file_path = None):
    if file_path is None:
        file_path = f"nnue/nnue_ep_{str(epoch).zfill(4)}.pt"
        
    checkpoint = {
        'epoch': epoch,
        'model_state_dict': model.state_dict(),
        'optimizer_state_dict': optimizer.state_dict()
    }
    torch.save(checkpoint, file_path)
    print(f"Checkpoint saved at epoch {epoch + 1}")

def load_checkpoint(file_path, model, optimizer = None):
    if os.path.exists(file_path):
        checkpoint = torch.load(file_path)
        model.load_state_dict(checkpoint['model_state_dict'])
        if optimizer:
            optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
            print(f"Checkpoint loaded from {file_path}, starting at epoch {checkpoint['epoch'] + 1}")
            return checkpoint['epoch'] + 1
        else:
            print(f"Checkpoint loaded from {file_path}, no optimizer")
            return checkpoint['epoch']
    else:
        print(f"No checkpoint found at {file_path}")
        return 0