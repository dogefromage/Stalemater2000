import datetime
import torch
from model import NNUE, load_checkpoint, input_size, hl_size

nnue = NNUE()

model_path = "nnue/nnue_ep_0120.pt"
load_checkpoint(model_path, nnue)

scale = 10000

def ensure_2d(tensor):
    if tensor.dim() == 0:  # Scalar case
        return tensor.unsqueeze(0).unsqueeze(0)
    elif tensor.dim() == 1:  # 1D case
        return tensor.unsqueeze(0)  # Make it row vector (1, N)
    return tensor  # Already 2D or higher

def save_params_raw(name, w):
    w = ensure_2d(w)
    with open(f"weights/{name}.bin", "wb") as f:
        w = (w * scale).to(torch.int32)
        rows, columns = w.shape
        print(name, rows, columns)
        # column major
        for i in range(columns):
            for j in range(rows):
                val = w[j][i].item()
                f.write((val).to_bytes(4, byteorder='big', signed=True))

def save_params_csv_float(name, w, file):
    # export in column major format
    w = ensure_2d(w).float()
    rows, columns = w.shape
    file.write(f"{name} {rows} {columns}\n")
    for i in range(columns):
        for j in range(rows):
            val = w[j][i].item()
            f.write(f"{val}\n")

with open(f"weights/nnue_{datetime.datetime.now()}.csv", "w") as f:
    for name, param in nnue.named_parameters():
        print(f"Exporting {name}")
        save_params_csv_float(name, param.data, f)
