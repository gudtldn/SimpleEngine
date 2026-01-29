import pandas as pd
import json
import matplotlib.pyplot as plt
import argparse
import os

def parse_args():
    parser = argparse.ArgumentParser(description="Google Benchmark 시각화 및 리포트 도구")
    parser.add_argument("input", help="입력 JSON 파일 경로")
    parser.add_argument("--output", "-o", help="결과 이미지 저장 경로 (입력하지 않으면 저장 안 함)", default=None)
    parser.add_argument("--filter", "-f", help="특정 이름 필터링", default="")
    return parser.parse_args()

def print_context_info(context):
    """빌드 환경 및 시스템 정보 출력"""
    print("\n" + "="*50)
    print(" [ SYSTEM & BUILD INFORMATION ]")
    print("-"*50)
    print(f"📅 Date       : {context.get('date', 'N/A')}")
    print(f"💻 CPU        : {context.get('host_name', 'Local Machine')}")
    print(f"🚀 Core/Freq  : {context.get('num_cpus', 0)} cores @ {context.get('mhz_per_cpu', 0)} MHz")
    print(f"📦 Cache L1/L2: {context.get('caches', [{}])[0].get('size', 'N/A')} / {context.get('caches', [{}])[1].get('size', 'N/A')}")
    print(f"⚠️ Build Type : {'DEBUG (Slow!)' if 'DEBUG' in str(context) else 'RELEASE (Optimal)'}")
    print("="*50 + "\n")

def load_data(filepath):
    if not os.path.exists(filepath):
        print(f"❌ Error: 파일을 찾을 수 없습니다: {filepath}")
        return None, None

    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    df = pd.DataFrame(data['benchmarks'])

    # 이름 파싱 (BaseName / Size)
    if df['name'].str.contains('/').any():
        split_data = df['name'].str.split('/', n=1, expand=True)
        df['base_name'] = split_data[0]
        df['size'] = pd.to_numeric(split_data[1], errors='coerce').fillna(0).astype(int)
    else:
        df['base_name'] = df['name']
        df['size'] = 0

    return df, data.get('context', {})

def plot_benchmark(df, context, output_path, filter_str):
    if filter_str:
        df = df[df['name'].str.contains(filter_str, case=False)]

    if df.empty:
        print("⚠️ 필터 결과가 없습니다.")
        return

    fig, ax1 = plt.subplots(figsize=(12, 7))
    ax2 = ax1.twinx()

    # 색상 사이클 설정 (항목별로 뚜렷한 색상 부여)
    colors = plt.cm.tab10.colors
    has_sizes = (df['size'] > 0).any()

    lines = [] # 범례를 하나로 합치기 위한 리스트

    if has_sizes:
        for i, (label, group) in enumerate(df.groupby('base_name')):
            group = group.sort_values('size')
            color = colors[i % len(colors)]

            # 1. 실선: CPU 시간 (색상별로 다른 테스트 항목 의미)
            ln1 = ax1.plot(group['size'], group['cpu_time'], marker='o',
                          label=f"{label} (Time)", color=color, linewidth=2)
            # 2. 점선: 이터레이션 (같은 색상이지만 투명도와 스타일로 구분)
            ln2 = ax2.plot(group['size'], group['iterations'], ls='--',
                          color=color, alpha=0.3, label=f"{label} (Iters)")
            lines.extend(ln1)

        ax1.set_xscale('log', base=2)
        ax1.set_yscale('log')
        ax1.set_xlabel('Problem Size (N)', fontsize=12)
    else:
        # 막대 그래프일 경우 각 막대에 이름을 써서 색상 의존도 낮춤
        bars = ax1.barh(df['name'], df['cpu_time'], color=plt.cm.Paired.colors)
        ax1.bar_label(bars, fmt='%.0f ns', padding=5)
        ax1.invert_yaxis()
        ax1.set_xlabel('Time (ns)', fontsize=12)

    # 범례 통합 (시간 정보만 깔끔하게 표시)
    if has_sizes:
        labs = [l.get_label() for l in lines]
        ax1.legend(lines, labs, loc='upper left', bbox_to_anchor=(1.15, 1), title="Benchmarks")

    ax1.set_ylabel('CPU Time (ns) [Solid Line]', fontsize=12, color='black')
    ax2.set_ylabel('Iterations [Faint Dash]', fontsize=12, color='gray')

    plt.title(f"Performance Report\nTarget: {filter_str if filter_str else 'All'}", fontsize=14)
    plt.grid(True, which="both", ls="-", alpha=0.2)
    fig.tight_layout()

    if output_path:
        plt.savefig(output_path)
    plt.show()

if __name__ == "__main__":
    args = parse_args()
    bench_df, context_info = load_data(args.input)

    if bench_df is not None:
        print_context_info(context_info)
        plot_benchmark(bench_df, context_info, args.output, args.filter)
