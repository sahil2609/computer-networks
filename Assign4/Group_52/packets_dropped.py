NewReno=[0.320976 ,1,
0.715856 ,2,
0.905392 ,3,
0.910112 ,4,
0.914832 ,5,
1.34349 ,6,]

Hybla=[0.320976 ,1,
0.719568 ,2,
0.92024 ,3,
0.928672 ,4,
0.933392 ,5,
1.41402 ,6,]

Westwood=[0.346528 ,1,
0.772496 ,2,
0.962032 ,3,
0.966752 ,4,
0.971472 ,5,
1.38528 ,6,]

Scalable=[0.320976 ,1,
0.719568 ,2,
0.905392 ,3,
0.913824 ,4,
0.922256 ,5,
1.38803 ,6,]

Vegas=[0.320976 ,1,
0.719568 ,2,
0.905392 ,3,
0.913824 ,4,
0.922256 ,5,]

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