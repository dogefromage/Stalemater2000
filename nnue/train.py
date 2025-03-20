import torch
from torch import nn
from model import save_checkpoint
from torch.utils.tensorboard import SummaryWriter
# from torch.utils.progress import ProgressBar
from tqdm import tqdm

device = "cuda" if torch.cuda.is_available() else "cpu"

def relative_threshold_accuracy(y_true, y_pred, rel_threshold=0.1, base_threshold=10):
    """
    Calculates ratio of correctly identified evaluations below difference of max(|y_pred| * rel_threhold, base_threshold)
    """
    threshold = torch.clamp_min(rel_threshold * torch.abs(y_true), base_threshold)

    correct_predictions = (torch.abs(y_pred - y_true) <= threshold).float()
    
    return correct_predictions.mean().item()


def train(dir_output, params, train_loader_creator, validation_loader_creator, model):
    print(f"Training using {device}")

    writer = SummaryWriter(log_dir=dir_output)
    layout = {
        "Training": {
            "loss": ["Multiline", ["loss/train", "loss/validation"]],
            "accuracy": ["Multiline", ["accuracy/train", "accuracy/validation"]],
            "hyperparams": ["Multiline", ["hyperparams/learning_rate"]],
        },
    }
    writer.add_custom_scalars(layout)

    num_epochs = params['num_epochs']
    batch_size = params['batch_size']
    num_training_slices = params['num_training_slices']

    model.to(device)

    optimizer = torch.optim.Adam(
        model.parameters(), lr=params['learning_rate'], weight_decay=params['weight_decay'])
    
    scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, mode='min', factor=0.5, patience=5, threshold=0.001)
    
    lossfn = nn.HuberLoss()

    for epoch in range(0, num_epochs):

        train_loaders_iter = train_loader_creator.dataloader_slices_iter(num_training_slices)
        for slice_index, train_loader in enumerate(train_loaders_iter):

            writer_step = epoch * num_training_slices + slice_index
            sub_epoch_name = f"{str(epoch).zfill(4)}-{slice_index}"

            model.train()
            train_acc_loss = 0.0
            train_acc_accuracy = 0.0
            train_samples = 0

            for train_batch in tqdm(train_loader, desc=f"Training {sub_epoch_name}"):
                inputs, true_evals, black_to_move, positions = train_batch

                inputs = inputs.to(device)
                black_to_move = black_to_move.to(device=device)
                true_evals = true_evals.to(device=device)

                estimate = model(inputs, black_to_move)
                loss = lossfn(estimate, true_evals)

                optimizer.zero_grad()
                loss.backward()

                torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
                optimizer.step()
                
                train_acc_loss += loss.item() * batch_size
                train_acc_accuracy += relative_threshold_accuracy(true_evals, estimate) * batch_size
                train_samples += batch_size

            train_loss = train_acc_loss / train_samples
            train_accuracy = train_acc_accuracy / train_samples
            
            writer.add_scalar("loss/train", train_loss, writer_step)
            writer.add_scalar("accuracy/train", train_accuracy, writer_step)
            print(f"Training loss={train_loss}, accuracy={train_accuracy}")

            # validate every training slice

            model.eval()
            valid_acc_loss = 0.0
            valid_acc_accuracy = 0.0
            valid_samples = 0

            valid_loader = validation_loader_creator.create_single_loader()
            for valid_batch in tqdm(valid_loader, desc=f"Validating {sub_epoch_name}"):
                inputs, true_evals, black_to_move, positions = valid_batch

                inputs = inputs.to(device)
                black_to_move = black_to_move.to(device=device)
                true_evals = true_evals.to(device=device)

                with torch.no_grad():
                    estimate = model(inputs, black_to_move)
                    loss = lossfn(estimate, true_evals)

                valid_acc_loss += loss.item() * batch_size
                valid_acc_accuracy += relative_threshold_accuracy(true_evals, estimate) * batch_size
                valid_samples += batch_size
                
            valid_loss = valid_acc_loss / valid_samples
            valid_accuracy = valid_acc_accuracy / valid_samples
            
            scheduler.step(valid_loss)
            writer.add_scalar("hyperparams/learning_rate", scheduler.get_last_lr()[0], writer_step)

            writer.add_scalar("loss/validation", valid_loss, writer_step)
            writer.add_scalar("accuracy/validation", valid_accuracy, writer_step)
            writer.flush()
            print(f"Validation loss={valid_loss}, accuracy={valid_accuracy}")

            checkpoint_path = dir_output / f"nnue_{sub_epoch_name}.pt"
            save_checkpoint(model, optimizer, epoch, checkpoint_path)

