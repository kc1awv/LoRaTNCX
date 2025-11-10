"""Simple config manager to save/load user settings to JSON."""
import json
from pathlib import Path


DEFAULT_FILE = Path.home() / ".lora_keyboard_chat_config.json"


def load_config(path: str | Path = None) -> dict:
    p = Path(path) if path else DEFAULT_FILE
    if not p.exists():
        return {}
    try:
        with open(p, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def save_config(cfg: dict, path: str | Path = None) -> bool:
    p = Path(path) if path else DEFAULT_FILE
    try:
        with open(p, "w", encoding="utf-8") as f:
            json.dump(cfg, f, indent=2)
        return True
    except Exception:
        return False
