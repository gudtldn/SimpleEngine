"""
compare_benchmarks.py — before/after JSON 비교 도구

사용법:
    python compare_benchmarks.py before.json after.json
    python compare_benchmarks.py before.json after.json --output result.png --filter MyBench
"""

import json
import os
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker


def parse_args():
    parser = argparse.ArgumentParser(description="Google Benchmark before/after 비교 도구")
    parser.add_argument("before", help="최적화 전 JSON 파일")
    parser.add_argument("after",  help="최적화 후 JSON 파일")
    parser.add_argument("--output", "-o", default=None, help="이미지 저장 경로")
    parser.add_argument("--filter", "-f", default="",   help="벤치마크 이름 필터 (정규식)")
    return parser.parse_args()


def load(filepath):
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"File not found: {filepath}")
    with open(filepath, encoding="utf-8") as f:
        data = json.load(f)
    df = pd.DataFrame(data["benchmarks"])[["name", "cpu_time"]]
    if df["name"].str.contains("/").any():
        split = df["name"].str.split("/", n=1, expand=True)
        df["base_name"] = split[0]
        df["size"] = pd.to_numeric(split[1], errors="coerce").fillna(0).astype(int)
    else:
        df["base_name"] = df["name"]
        df["size"] = 0
    return df


def build_comparison(df_before, df_after, filter_str):
    if filter_str:
        df_before = df_before[df_before["name"].str.contains(filter_str, case=False)]
        df_after  = df_after[df_after["name"].str.contains(filter_str, case=False)]

    merged = df_before.merge(df_after, on="name", suffixes=("_before", "_after"))
    merged["speedup"]  = merged["cpu_time_before"] / merged["cpu_time_after"]
    merged["base_name"] = merged["base_name_before"]
    merged["size"]      = merged["size_before"]
    return merged


def plot(merged, output_path):
    has_sizes = (merged["size"] > 0).any()
    colors    = plt.cm.tab10.colors

    if not has_sizes:
        # 크기 변수 없을 때: 단순 가로 막대 speedup
        fig, ax = plt.subplots(figsize=(10, max(4, len(merged) * 0.4 + 2)))
        fig.suptitle("Before vs After - Speedup", fontsize=13, fontweight="bold")
        bars = ax.barh(merged["name"], merged["speedup"],
                       color=[colors[i % len(colors)] for i in range(len(merged))])
        ax.bar_label(bars, fmt="%.2fx", padding=4, fontsize=8)
        ax.axvline(1.0, color="gray", ls=":", linewidth=1.2)
        ax.invert_yaxis()
        ax.set_xlabel("Speedup  (before / after)", fontsize=11)
        ax.grid(True, axis="x", alpha=0.2)
        fig.tight_layout()
        if output_path:
            plt.savefig(output_path, bbox_inches="tight", dpi=150)
            print(f"Saved: {output_path}")
        plt.show()
        return

    # base_name의 마지막 _ 로 (group, label) 분리 — 표시용 휴리스틱
    # 예) BM_Array_InsertFront_LargePod → group=BM_Array_InsertFront, label=LargePod
    merged = merged.copy()
    parts             = merged["base_name"].str.rsplit("_", n=1, expand=True)
    merged["group"]   = parts[0]
    merged["label"]   = parts[1]

    groups = list(dict.fromkeys(merged["group"]))   # 순서 유지
    labels = list(dict.fromkeys(merged["label"]))
    label_color = {lb: colors[i % len(colors)] for i, lb in enumerate(labels)}

    n_cols = len(groups)
    fig, axes = plt.subplots(2, n_cols, figsize=(5.5 * n_cols, 8), squeeze=False)

    # 공통 제목 + 범례 설명
    fig.suptitle("Before vs After\n"
                 "top: CPU time  (solid = before,  dashed = after)  |  "
                 "bottom: speedup  (>1.0 = faster after)",
                 fontsize=11, fontweight="bold")

    for col, grp in enumerate(groups):
        ax_t = axes[0][col]
        ax_s = axes[1][col]

        # 서브플롯 제목: 공통 접두사(BM_/BM_XXX_) 제거
        title = grp
        for prefix in ("BM_Array_", "BM_SE_", "BM_STL_", "BM_"):
            title = title.replace(prefix, "")
        ax_t.set_title(title, fontsize=11, fontweight="bold")

        ax_s.axhline(1.0, color="gray", ls=":", linewidth=1.2, zorder=0)
        ax_s.set_xlabel("N", fontsize=9)

        sub = merged[merged["group"] == grp]
        for lb in labels:
            g = sub[sub["label"] == lb].sort_values("size")
            if g.empty:
                continue
            c = label_color[lb]
            ax_t.plot(g["size"], g["cpu_time_before"], marker="o", color=c,
                      linewidth=2, label=lb)
            ax_t.plot(g["size"], g["cpu_time_after"], marker="o", color=c,
                      linewidth=1.3, ls="--", alpha=0.45)
            ax_s.plot(g["size"], g["speedup"], marker="s", color=c,
                      linewidth=2, label=lb)

        for ax in (ax_t, ax_s):
            ax.set_xscale("log", base=2)
            ax.grid(True, which="both", alpha=0.15)
            ax.legend(fontsize=8, loc="lower right", framealpha=0.88)

        ax_t.set_yscale("log")
        ax_s.set_ylim(bottom=0)

        if col == 0:
            ax_t.set_ylabel("CPU Time (ns)  [lower = better]", fontsize=10)
            ax_s.set_ylabel("Speedup  [higher = better]", fontsize=10)

    fig.tight_layout()

    if output_path:
        plt.savefig(output_path, bbox_inches="tight", dpi=150)
        print(f"Saved: {output_path}")
    plt.show()


if __name__ == "__main__":
    args = parse_args()
    df_before = load(args.before)
    df_after  = load(args.after)
    merged    = build_comparison(df_before, df_after, args.filter)

    if merged.empty:
        print("No matching benchmarks found.")
        exit(1)

    # 터미널 요약 출력
    print(f"\n{'Benchmark':<55} {'Before':>10} {'After':>10} {'Speedup':>8}")
    print("-" * 88)
    for _, row in merged.iterrows():
        label = row["name"] if row["size"] == 0 else f"{row['base_name']}/{row['size']}"
        print(f"{label:<55} {row['cpu_time_before']:>9.0f}ns {row['cpu_time_after']:>9.0f}ns  {row['speedup']:>6.2f}x")

    plot(merged, args.output)