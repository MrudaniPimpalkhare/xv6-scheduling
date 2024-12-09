import matplotlib.pyplot as plt
import csv
from collections import defaultdict
import numpy as np

def parse_mlfq_data(filename):
    processes = defaultdict(list)
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            if len(row) >= 3:
                event, pid, time = row[:3]
                if event == 'MLFQ' and len(row) == 4:
                    _, pid, time, priority = row
                    processes[pid].append((int(time), int(priority)))
    return processes

def plot_mlfq_graph(processes):
    plt.figure(figsize=(12, 6))
    colors = plt.cm.rainbow(np.linspace(0, 1, len(processes)))
    
    for (pid, events), color in zip(processes.items(), colors):
        times, priorities = zip(*[(t, p if isinstance(p, int) else -1) for t, p in events])
        plt.scatter(times, priorities, c=[color], label=f'Process {pid}', s=20)
        plt.plot(times, priorities, c=color, alpha=0.3)

    plt.xlabel('Number of ticks')
    plt.ylabel('Queue priority')
    plt.yticks(range(4), ['Q0', 'Q1', 'Q2', 'Q3'])
    plt.title('MLFQ Process Queue Timeline')
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    plt.grid(True, which='both', linestyle='--', linewidth=0.5)
    plt.savefig('mlfq_graph2.png', dpi=300, bbox_inches='tight')
    plt.show()

if __name__ == '__main__':
    processes = parse_mlfq_data('log.txt')
    plot_mlfq_graph(processes)