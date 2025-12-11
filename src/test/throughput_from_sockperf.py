from pathlib import Path
import matplotlib.pyplot as plt
import csv

MSG_SIZE_BYTES = 1024


def load_latencies(path):
    latencies = []
    with open(path, newline="") as f:
        reader = csv.reader(f)

        header_found = False
        for row in reader:
            if not row:
                continue

            first = row[0].strip()

            if first.startswith("packet"):
                header_found = True
                continue

            if not header_found:
                continue

            if first.startswith("bin"):
                break

            try:
                latency_usec = float(row[3])
            except (IndexError, ValueError):
                continue

            latencies.append(latency_usec)

    return latencies


def latencies_to_throughput_kBps(latencies):
    throughputs = []
    for us in latencies:
        if us <= 0:
            continue
        kbps = (MSG_SIZE_BYTES / (us * 1e-6)) / 1024.0
        throughputs.append(kbps)

    throughputs.sort(reverse=True)
    return throughputs


def plot_throughput(values, title, output_path, label, color):
    if not values:
        raise ValueError(f"No latency samples found for {label}")

    x_axis = range(1, len(values) + 1)
    plt.figure(figsize=(8, 5))
    plt.scatter(x_axis, values, s=5, color=color, label=label)

    plt.title(title)
    plt.xlabel("Request #")
    plt.ylabel("Throughput (KB/s)")
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()

    print(f"Saved throughput plot: {output_path}")


def main():
    base_dir = Path(__file__).resolve().parent
    direct_path = base_dir / "latency_direct.csv"
    proxy_path = base_dir / "latency_via_proxy.csv"

    direct_output = base_dir / "throughput_direct.png"
    proxy_output = base_dir / "throughput_via_proxy.png"

    lat_direct = load_latencies(direct_path)
    lat_proxy = load_latencies(proxy_path)

    thr_direct = latencies_to_throughput_kBps(lat_direct)
    thr_proxy = latencies_to_throughput_kBps(lat_proxy)

    plot_throughput(
        thr_direct,
        "SOCKS5 Throughput - Direct connection",
        direct_output,
        "Direct",
        "#1f77b4",
    )

    plot_throughput(
        thr_proxy,
        "SOCKS5 Throughput - Via proxy",
        proxy_output,
        "Proxy",
        "#d62728",
    )


if __name__ == "__main__":
    main()
