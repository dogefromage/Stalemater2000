import datetime
import torch
from model import NNUE, load_checkpoint, input_size, hl_size

source_file_path = "/home/seb/git/Stalemater2000/src/nnue_data.cpp"
model_path = "/home/seb/git/Stalemater2000/nnue/output/exp_2025-03-21_15-28-50/nnue_0001-9.pt"

template = """
#include "nnue.h"

float nnue_accumulator_weights[INPUT_SIZE][HL_SIZE] = %%accumulation_layer.weight%%;
float nnue_accumulator_biases[HL_SIZE] = %%accumulation_layer.bias%%;
float nnue_output_weights[NUM_BUCKETS][2 * HL_SIZE] = %%output_layer.weight%%;
float nnue_output_bias[NUM_BUCKETS] = %%output_layer.bias%%;

"""

nnue = NNUE()
load_checkpoint(model_path, nnue)

def array_initializer_of_1d(tensor_1d: torch.tensor):
    expr = ", ".join([ str(w.item()) for w in tensor_1d.flatten() ])
    expr = "{ " + expr + " }"
    return expr

def array_initializer_of_2d(tensor_2d: torch.tensor):
    hl_vector_strs = [ array_initializer_of_1d(x) for x in param_tensor.T ]
    return "{ " + ", ".join(hl_vector_strs) + " }"


for name, param_tensor in nnue.named_parameters():
    print(f"Exporting {name}")

    if name == 'accumulation_layer.weight':
        initializer = array_initializer_of_2d(param_tensor.T)
    if name == 'output_layer.weight':
        initializer = array_initializer_of_2d(param_tensor)
    else:
        initializer = array_initializer_of_1d(param_tensor)

    template = template.replace(f"%%{name}%%", initializer)

with open(source_file_path, "w") as f:
    f.write(template)
