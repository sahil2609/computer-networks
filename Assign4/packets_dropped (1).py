NewReno=[0,1,0.797296 ,2,
0.809408 ,3,
0.854128 ,4,
0.888848 ,5,
1.36947 ,6]

Hybla=[0,1,0.601008 ,2,
0.775696 ,3,
0.884128 ,4,
0.888848 ,5,
1.29894 ,6]

Westwood=[0,1,0.803845 ,2,
0.985957 ,3,
0.990677 ,4,
0.995397 ,5,
1.41292 ,6]

Scalable=[0,1,0.701008 ,2,
0.875696 ,3,
0.880416 ,4,
0.885136 ,5]

Vegas=[0,1,0.701008 ,2,
0.775486 ,3,
0.880416 ,4,
0.985136 ,5,
1.30266 ,6]

nren1 = []
nren2 = []
hyb1 = []
hyb2 = []
wwood1 = []
wwood2 = []
sca1 = [] 
sca2 = []
veg1 = []
veg2 = []

import matplotlib.pyplot as plt

for i in range(int(len(NewReno)/2)):
    nren1.append(NewReno[2*i])
    nren2.append(NewReno[2*i+1]-1)

for i in range(int(len(Hybla)/2)):
    hyb1.append(Hybla[2*i])
    hyb2.append(Hybla[2*i+1]-1)

for i in range(int(len(Westwood)/2)):
    wwood1.append(Westwood[2*i])
    wwood2.append(Westwood[2*i+1]-1)

for i in range(int(len(Scalable)/2)):
    sca1.append(Scalable[2*i])
    sca2.append(Scalable[2*i+1]-1)

for i in range(int(len(Vegas)/2)):
    veg1.append(Vegas[2*i])
    veg2.append(Vegas[2*i+1]-1)



plt.plot(nren1,nren2, label='TCP New Reno')  
plt.plot(hyb1,hyb2, label='TCP Hybla')
plt.plot(wwood1,wwood2, label='TCP Westwood')
plt.plot(sca1,sca2, label='TCP Scalable')
plt.plot(veg1,veg2, label='Vegas')
plt.title("cumulative TCP packets dropped w.r.t. time")
plt.ylabel("No. of packets lost")
plt.xlabel("Time (in seconds)")
plt.legend(loc='upper left')
plt.show()