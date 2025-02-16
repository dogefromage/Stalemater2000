import os
from torch import nn
import torch

class SCReLU(nn.Module):
    def __init__(self):
        super().__init__()

    def forward(self, x):
        return torch.clamp(x, min=0, max=1) ** 2
    
input_size = 768
hl_size = 1024

class NNUE(nn.Module):
    def __init__(self):
        super().__init__()
        self.input_dropout = nn.Dropout(0.01)
        self.accumulation_layer = nn.Linear(input_size, hl_size, bias=True)
        self.activation = SCReLU()
        self.accumulation_dropout = nn.Dropout(0.1)
        self.output_layer = nn.Linear(2 * hl_size, 1, bias=True)

    def forward(self, x, black_to_move):

        x = self.input_dropout(x)

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
        acc = self.accumulation_dropout(acc)

        output = self.output_layer(acc)

        # mirror evaluation
        output = torch.where(black_to_move, -output, output)

        return output

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
            return -1
    else:
        print(f"No checkpoint found at {file_path}")
        return 0