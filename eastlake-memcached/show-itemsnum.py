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
x = 0
while True:
    x_list.append(x)
    y = float(mc.get_stats()[0][1]['curr_items'])
    y = y/1000000
    y_list.append(y)
    plt.plot(x_list, y_list,'b--',mfc='w')
    plt.xlabel("Time(s)")
    plt.ylabel("number(M)")
    plt.title("memcached curr_items")
    plt.show()
    plt.pause(1)
    x += 1

    
