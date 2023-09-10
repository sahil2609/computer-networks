import matplotlib.pyplot as plt
import numpy as np
import matplotlib.pyplot as plt
import statistics

f= np.loadtxt('data2.txt', unpack='False')
print(statistics.median(f))

# set bins' interval for your data
# You have following intervals: 
# 1st col is number of data elements in [0,10000);
# 2nd col is number of data elements in [10000, 20000); 
# ...
# last col is number of data elements in [100000, 200000]; 
bins = [0,10000,20000,50000,100000,200000] 

plt.hist(f, bins = 400, rwidth=2)
plt.xlabel('Latency in ms')
plt.ylabel('Frequency')
plt.title('$ping -p ff00 172.17.0.22')
plt.legend()
plt.show()