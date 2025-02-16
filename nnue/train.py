import torch
from torch import nn
from model import NNUE, save_checkpoint, load_checkpoint

device = "cuda" if torch.cuda.is_available() else "cpu"
print(f"Torch using {device}")

boards, black_to_move, evals  = torch.load("./datasets/dataset_1000000.pt")
print(len(boards))
print("Dataset loaded")

class Dataset(torch.utils.data.Dataset):
    def __init__(self, boards, black_to_move, evals):
        self.boards = boards
        self.black_to_move = black_to_move 
        self.evals = evals

    def __len__(self):
        return len(self.boards)

    def __getitem__(self, idx):
        return self.boards[idx], self.black_to_move[idx], self.evals[idx]
    
training_data = Dataset(boards, black_to_move, evals)

nnue = NNUE().to(device)

learning_rate = 0.001
batchsize = 64

train_dataloader = torch.utils.data.DataLoader(training_data, batch_size=batchsize, shuffle=True)

lossfn = nn.MSELoss()
optimizer = torch.optim.Adam(nnue.parameters(), learning_rate)
num_epochs = 100

epoch = 0
# epoch = load_checkpoint('nnue/nnue_ep_0039.pt', nnue, optimizer)

while epoch < num_epochs:
    nnue.train()
    running_loss = 0.0
    total_samples = 0

    for batch in train_dataloader:
        boards, black_to_move, evals = batch

        boards = boards.to(dtype=torch.float, device=device)
        black_to_move = black_to_move.to(dtype=torch.bool, device=device)
        evals = evals.to(dtype=torch.float, device=device)

        guess = nnue(boards, black_to_move)
        loss = lossfn(guess, evals)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()
        
        running_loss += loss.item() * batchsize
        total_samples += batchsize

    print(f"Epoch {epoch+1}/{num_epochs}, Running loss: {running_loss/total_samples}")
    save_checkpoint(nnue, optimizer, epoch)

    epoch += 1