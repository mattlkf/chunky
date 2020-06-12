#!/usr/bin/python

import sys
import math
import re
import matplotlib.pyplot as plt

from collections import defaultdict

assert len(sys.argv) > 1

fname = sys.argv[1]

with open(fname, "r") as f:
    lines = [line.strip() for line in f.readlines()]


impt_lines = [line.split(' ') for line in lines if line.split(' ')[0] == "[["]

def get_chunkserver_index(line):
    chunkserver = line[1]
    parts = re.split('_|:', chunkserver)
    return int(parts[1])

def get_time(line):
    return int(line[2])

# print(impt_lines)
# print(get_chunkserver_index("chunk_2:223"))

# data = {get_chunkserver_index(line):get_time(line) for line in impt_lines}

data = defaultdict(list)

for line in impt_lines:
    data[get_chunkserver_index(line)].append(get_time(line))

print(data)

def activity_per_interval(data,interval):
    # Find the max time
    maxtime = 0
    for k,v in data.items():
        maxtime = max(maxtime, max(v))
    print(maxtime)

    n_intervals = math.ceil(maxtime / interval)
    print(n_intervals)

    activity = {}
    for k,v in data.items():
        activity[k] = [0 for _ in range(n_intervals)]
        for x in v:
            activity[k][math.floor(x/interval)] += 1
    return activity, interval, n_intervals


activity, interval, n_intervals = activity_per_interval(data, 1000 * 1000)

cm = plt.get_cmap('gist_rainbow')
n_colors = len(activity.items())
colors = [cm(1.*i/n_colors) for i in range(n_colors)]
print(colors)

stack_y = []
for k in range(1,n_colors+1):
    stack_y.append(activity[k])
    plt.plot([],[],color=colors[k-1], label = "chunkserver_"+str(k))

stack_x = [i * interval for i in range(n_intervals)]

plt.stackplot(stack_x, stack_y,colors=colors)
plt.legend()

plt.ylabel("Requests served per second")

plt.xlabel("Interval")
plt.show()
