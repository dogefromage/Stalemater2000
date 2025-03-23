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
