import pytest
import chess.engine
from omegaconf import OmegaConf

ROOT_CONFIG = "./root_config.yml"
TEST_CONFIG = "./tests/test_config.yml"
root_config = OmegaConf.load(ROOT_CONFIG)
test_config = OmegaConf.load(TEST_CONFIG)
config = OmegaConf.merge(root_config, test_config)

@pytest.fixture(scope="module")
def engine():
    engine = chess.engine.SimpleEngine.popen_uci(config.path_executable)
    yield engine
    engine.quit()

def test_engine_load(engine):
    assert engine is not None, "Engine failed to load."

def test_engine_position_evaluation(engine):
    for fen in config.test_positions.positions:
        print(f"Testing {fen}")
        board = chess.Board(fen)
        result = engine.play(board, chess.engine.Limit(time=config.test_positions.limit))
        assert result.move is not None, f"Engine failed to return a move for position {fen}"

# def test_perft(engine):
#     """Test if the engine correctly evaluates perft scores."""
#     for test_case in config.perft_tests:
#         board = chess.Board(test_case.fen)
#         depth = test_case.depth
#         expected_perft = test_case.expected
        
#         engine.protocol.send_line(f"position fen {test_case.fen}")
#         engine.protocol.send_line(f"go perft {depth}")
        
#         perft_results = {}
#         while True:
#             response = engine.protocol.recv_line()
#             if "->" in response:
#                 move, nodes = response.split(" -> ")
#                 perft_results[move.strip()] = int(nodes.strip())
#             if "Total" in response:
#                 break
        
#         assert perft_results == expected_perft, f"Perft mismatch for {test_case.fen} at depth {depth}. Expected: {expected_perft}, Got: {perft_results}"
