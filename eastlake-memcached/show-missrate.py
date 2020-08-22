#!/usr/bin/python
# -*- coding: UTF-8 -*-
# refer: https://github.com/linsomniac/python-memcached/blob/master/memcache.py
import memcache
import matplotlib.pyplot as plt

mc = memcache.Client(["127.0.0.1:11211"],debug=True)

plt.ion()
plt.figure(1)
x_list = []
y_list = []
file_x = open("missrate_time", "w")
file_y = open("missrate", "w")
x = 0
while True:
    x_list.append(x)
    cmd_get = float(mc.get_stats()[0][1]['cmd_get'])
    get_misses = float(mc.get_stats()[0][1]['get_misses'])
    if cmd_get != 0:
	y = get_misses/cmd_get
    else:
	y = 0
    y = y * 100
    y_list.append(y)
    file_x.write(str(x)+"\n")
    file_y.write(str(y)+"\n")
    plt.plot(x_list, y_list,'b--',mfc='w')
    plt.xlabel("Time(s)")
    plt.ylabel("miss rate")
    plt.title("memcached get miss rate")
    plt.show()
    plt.pause(1)
    x += 1
