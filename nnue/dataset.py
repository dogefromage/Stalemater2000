import io
import torch as pt
import zstandard as zstd
import json
import re

from position import Position

def read_record(record):
    fen = record['fen']
    deepest_eval = sorted(record['evals'], key=lambda x: x['depth'], reverse=True)[0]
    first_pv = deepest_eval['pvs'][0]

    side_to_move = re.search("^[\\w\\d/]+ (w|b)", fen).group(1)

    fixed_eval = 0
    if 'mate' in first_pv:
        mate_value = first_pv['mate']
        if mate_value < 0:
            fixed_eval = -5000
        else:
            fixed_eval = 5000
    else:
        centipawns = first_pv['cp']
        fixed_eval = centipawns

    return fen, fixed_eval, side_to_move
   
# def iterate_json_zst(path_zst):
#   with open(path_zst, 'rb') as file:
#     decompressor = zstd.ZstdDecompressor()
#     stream_reader = decompressor.stream_reader(file)
#     stream = io.TextIOWrapper(stream_reader, encoding = "utf-8")
#     for line in stream:
#       yield json.loads(line)

def iterate_full_dataset(path_zst):
    with open(path_zst, 'rb') as file:
        decompressor = zstd.ZstdDecompressor()
        stream_reader = decompressor.stream_reader(file)
        stream = io.TextIOWrapper(stream_reader, encoding = "utf-8")
        for line in stream:
            yield line

def iterate_next_n(iter, n):
    for _ in range(n):
        val = next(iter, None)
        if val is None:
            break
        yield val


class StreamDataset(pt.utils.data.IterableDataset):
    def __init__(self, data_iterator, length):
        super().__init__()
        self.data_iterator = data_iterator
        self.length = length
    
    def __len__(self):
        return self.length
    
    # def __iter__(self):
    #     return iter(self.data_iterator)

    def __iter__(self):
        worker_info = pt.utils.data.get_worker_info()
        if worker_info is None:
            return iter(self.data_iterator)
        else:
            worker_id = worker_info.id
            num_workers = worker_info.num_workers
            print(worker_id, num_workers)
            return (x for i, x in enumerate(self.data_iterator) if i % num_workers == worker_id)

def position_from_line(line):
    record = json.loads(line)
    fen, fixed_eval, side_to_move = read_record(record)
    position = Position(fen, fixed_eval, side_to_move)
    return position
    
def collate_data(lines):
    positions = list(map(position_from_line, lines))
    inputs = [ p.inputs for p in positions ]
    inputs = pt.stack(inputs).to(dtype=pt.float32)
    evals = [ pt.Tensor([p.fixed_eval]) for p in positions ]
    evals = pt.stack(evals).to(dtype=pt.float32)
    black_to_move = [ pt.Tensor([p.side_to_move == 'b']) for p in positions ]
    black_to_move = pt.stack(black_to_move).to(dtype=pt.bool)

    assert inputs.dtype == pt.float32
    assert evals.dtype == pt.float32
    assert black_to_move.dtype == pt.bool

    return inputs, evals, black_to_move, positions

class DataloaderCreator():
    def __init__(self, path_zst, length, batch_size, num_workers):
        self.path_zst = path_zst
        self.length = length
        self.batch_size = batch_size
        self.num_workers = num_workers

    def create_single_loader(self):
        iterator = iterate_full_dataset(self.path_zst)
        dataset = StreamDataset(iterator, self.length)
        dataloader = pt.utils.data.DataLoader(dataset, batch_size=self.batch_size, 
            collate_fn=collate_data, pin_memory=True, num_workers=self.num_workers)
        return dataloader
    
    def dataloader_slices_iter(self, num_slices):
        slice_length = self.length // num_slices
        data_iter = iterate_full_dataset(self.path_zst)

        for _ in range(num_slices):
            slice_iterator = iterate_next_n(data_iter, slice_length)
            dataset_slice = StreamDataset(slice_iterator, slice_length)
            dataloader = pt.utils.data.DataLoader(dataset_slice, batch_size=self.batch_size, collate_fn=collate_data, pin_memory=True, num_workers=self.num_workers)
            yield dataloader