#!/usr/bin/env python3
"""Generate a synthetic trades CSV for project A.

    python3 stage1/trade_stats/data/gen_trades.py --rows 1000000 --out data/big_trades.csv
"""
import argparse
import random


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rows", type=int, default=1000)
    ap.add_argument("--symbols", type=int, default=5)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--shuffle", action="store_true", help="shuffle rows so timestamps are out of order")
    ap.add_argument("--dirty", type=float, default=0.0, help="fraction of rows to corrupt, e.g. 0.01")
    ap.add_argument("--out", default="-")
    a = ap.parse_args()

    rng = random.Random(a.seed)
    names = [f"SYM{i:03d}" for i in range(a.symbols)]
    price = {n: rng.uniform(5, 200) for n in names}
    ts = 1_700_000_000_000_000_000

    rows = []
    for _ in range(a.rows):
        n = rng.choice(names)
        price[n] = max(0.01, price[n] * (1 + rng.gauss(0, 0.002)))
        ts += rng.randint(1_000, 5_000_000)
        rows.append(f"{ts},{n},{price[n]:.4f},{rng.randint(1, 500)}")

    if a.shuffle:
        rng.shuffle(rows)
    if a.dirty > 0:
        for i in range(len(rows)):
            if rng.random() < a.dirty:
                rows[i] = rng.choice(["", "garbage", rows[i].rsplit(",", 1)[0], "x," + rows[i]])

    out = open(a.out, "w") if a.out != "-" else None
    w = (lambda s: out.write(s + "\n")) if out else print
    w("timestamp_ns,symbol,price,qty")
    for r in rows:
        w(r)
    if out:
        out.close()


if __name__ == "__main__":
    main()
