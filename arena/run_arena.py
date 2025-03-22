import chess
import chess.engine
import time
import os
import pandas as pd
import random
from omegaconf import OmegaConf
from tqdm import tqdm

def get_engines_from_folder(folder_path):
    print(f"Looking for engines in folder: {folder_path}")
    if not os.path.exists(folder_path):
        raise FileNotFoundError(f"Engine folder '{folder_path}' not found.")
    
    engines = []
    for file in os.listdir(folder_path):
        full_path = os.path.join(folder_path, file)
        if os.path.isfile(full_path) and os.access(full_path, os.X_OK):
            engines.append(full_path)
    
    if not engines:
        raise ValueError("No executable chess engines found in the given folder.")
    
    print(f"Found {len(engines)} engines.")
    return sorted(engines)

def load_previous_results(csv_path):
    print(f"Loading previous results from {csv_path}")
    if os.path.exists(csv_path):
        return pd.read_csv(csv_path)
    print("No previous results found. Starting fresh.")
    return pd.DataFrame(columns=["Engine1", "Engine2", "Games", "Mean Result"])

def play_game(engine_white, engine_black, time_limit, depth):
    engine_white_name = os.path.basename(engine_white)
    engine_black_name = os.path.basename(engine_black)
    print(f"Starting game: {engine_white_name} (White) vs {engine_black_name} (Black)")
    board = chess.Board()
    
    try:
        with chess.engine.SimpleEngine.popen_uci(engine_white) as engine1, chess.engine.SimpleEngine.popen_uci(engine_black) as engine2:
            engines = [engine1, engine2]
            move_count = 0

            progress_bar = tqdm(total=100)
            
            while not board.is_game_over():
                engine = engines[move_count % 2]
                current_engine_name = engine_white_name if move_count % 2 == 0 else engine_black_name

                # print(f"Move {move_count+1}: {current_engine_name} is thinking...")
                if depth:
                    result = engine.play(board, chess.engine.Limit(depth=depth))
                else:
                    result = engine.play(board, chess.engine.Limit(time=time_limit))
                
                board.push(result.move)
                # print(f"Move {move_count+1}: {result.move} played")
                # print(board, "\n")
                time.sleep(0.1)
                move_count += 1
                progress_bar.update(1)

            progress_bar.close()
            
            result = board.result()
            print(f"Game finished. Result: {result}")
            
            if result == "1-0":
                return -1
            elif result == "0-1":
                return 1
            else:
                return 0
    except Exception as e:
        print(f"Error occurred during game: {e}")
        exit(1)

def compare_engines(config):
    print("Starting engine comparison...")
    engines = get_engines_from_folder(config.engine_folder)
    prev_results = load_previous_results(config.csv_path)
    new_results = {}
    
    for i in range(len(engines)):
        for j in range(i + 1, len(engines)):
            engine1, engine2 = engines[i], engines[j]
            engine1_name = os.path.basename(engine1)
            engine2_name = os.path.basename(engine2)
            engine1_name, engine2_name = sorted([engine1_name, engine2_name])
            
            existing_match = prev_results[(prev_results["Engine1"] == engine1_name) & (prev_results["Engine2"] == engine2_name)]
            if not existing_match.empty:
                print(f"Skipping {engine1_name} vs {engine2_name}, already recorded.")
                continue  # Skip if match already exists
            
            print(f"Comparing {engine1_name} vs {engine2_name} ({config.games_per_pair} games)")
            total_score = 0
            for game_number in range(config.games_per_pair):
                try:
                    print(f"Starting game {game_number + 1} between {engine1_name} and {engine2_name}")
                    if random.choice([True, False]):
                        score = play_game(engine1, engine2, config.time_limit, config.depth)
                    else:
                        score = -play_game(engine2, engine1, config.time_limit, config.depth)
                    total_score += score
                except Exception as e:
                    print(f"Error playing game between {engine1_name} and {engine2_name}: {e}")
            
            mean_score = total_score / config.games_per_pair
            print(f"Match {engine1_name} vs {engine2_name} completed. Mean result: {mean_score:.2f}")
            new_results[(engine1_name, engine2_name)] = (config.games_per_pair, mean_score)
    
    if new_results:
        new_results_df = pd.DataFrame([{ "Engine1": k[0], "Engine2": k[1], "Games": v[0], "Mean Result": v[1] } for k, v in new_results.items()])
        updated_results = pd.concat([prev_results, new_results_df], ignore_index=True)
        updated_results.to_csv(config.csv_path, index=False)
        print("Results saved to", config.csv_path)
    else:
        updated_results = prev_results
        print("No new results to save.")
    
    print("\nComparison Table:")
    engines_names = [os.path.basename(e) for e in engines]
    table = pd.DataFrame("", index=engines_names, columns=engines_names)
    
    for _, row in updated_results.iterrows():
        table.at[row["Engine1"], row["Engine2"]] = f"{row['Games']} ({row['Mean Result']:.2f})"
    
    sorted_engines = updated_results.groupby("Engine1")["Mean Result"].sum().sort_values(ascending=False).index.tolist()
    table = table.loc[sorted_engines, sorted_engines]
    print(table)
    return updated_results

if __name__ == "__main__":
    path_root_config = "./root_config.yml"
    path_arena_config = "./arena/arena_config.yml"
    root_config = OmegaConf.load(path_root_config)
    arena_config = OmegaConf.load(path_arena_config)
    config = OmegaConf.merge(root_config, arena_config)
    df_results = compare_engines(config)
    print("Final results:")
    print(df_results)
