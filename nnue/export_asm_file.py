import struct
import torch
from model import NNUE, load_checkpoint

assembly_file_path = "/home/seb/git/Stalemater2000/src/nnue_data.s"
model_path = "/home/seb/git/Stalemater2000/nnue/output/exp_2025-03-21_15-28-50/nnue_0001-9.pt"

assembly_template = """
.section .rodata
.global nnue_accumulator_weights
.global nnue_accumulator_biases
.global nnue_output_weights
.global nnue_output_bias
nnue_accumulator_weights:
    %%accumulation_layer.weight%%
nnue_accumulator_biases:
    %%accumulation_layer.bias%%
nnue_output_weights:
    %%output_layer.weight%%
nnue_output_bias:
    %%output_layer.bias%%
"""

nnue = NNUE()
load_checkpoint(model_path, nnue)

def float_to_hex(f):
    """Converts a float to IEEE-754 hexadecimal representation (32-bit single precision)."""
    return f"0x{struct.unpack('<I', struct.pack('<f', f))[0]:08x}"

def float_to_str(f):
    return str(f)

def array_to_assembly(tensor, label):
    elements = [float_to_str(v.item()) for v in tensor.flatten()]
    return f".float " + ", ".join(elements)

for name, param_tensor in nnue.named_parameters():
    print(f"Exporting {name}")
    
    if name == 'accumulation_layer.weight':
        initializer = array_to_assembly(param_tensor.T, "nnue_accumulator_weights")
    elif name == 'output_layer.weight':
        initializer = array_to_assembly(param_tensor, "nnue_output_weights")
    else:
        initializer = array_to_assembly(param_tensor, f"nnue_{name.replace('.', '_')}")
    
    assembly_template = assembly_template.replace(f"%%{name}%%", initializer)

with open(assembly_file_path, "w") as f:
    f.write(assembly_template)

print(f"Assembly file written to {assembly_file_path}")
