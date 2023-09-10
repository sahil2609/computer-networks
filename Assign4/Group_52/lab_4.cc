#include "ns3/internet-module.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include <iostream>
#include <fstream>
#include <string>
#include "ns3/packet-sink.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/error-model.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"


using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("Networks Lab");
AsciiTraceHelper ascii;
Ptr<PacketSink> cbr[5], sinkTcp;
int total_bytes_transfer = 0;
int total_packet_drops = 0;

//function to record Congestion Window values
static void CwndChange(Ptr<OutputStreamWrapper>stream, uint32_t oldCwnd, uint32_t newCwnd){
    *stream->GetStream() << Simulator::Now().GetSeconds() << " ," << oldCwnd << " ," << newCwnd << ",\n";
}

//function to find the total cumulative transfered bytes
static void TotalRx(Ptr<OutputStreamWrapper>stream){
    total_bytes_transfer+=sinkTcp->GetTotalRx();
    int i = 0;
    while(i<5){
        total_bytes_transfer += cbr[i]->GetTotalRx(); i++;
    }
    *stream->GetStream() << Simulator::Now().GetSeconds() << " ," << total_bytes_transfer << ",\n";
    Simulator::Schedule(Seconds(0.001),&TotalRx, stream);
}
//function to record packet drops
static void RxDrop(Ptr<OutputStreamWrapper>stream, Ptr<const Packet>p){
    total_packet_drops+=1;
    *stream->GetStream()<<Simulator::Now().GetSeconds()<< " ,"<<total_packet_drops << ",\n";
}

// Trace Congestion window length
static void TraceCwnd(Ptr<OutputStreamWrapper>stream){
    //Trace changes to the congestion window
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange,stream));
}

int main(int argc, char *argv[]){
    uint32_t mxbytes = 0;
    CommandLine cmd;
    string tcp_protocol_name = "TcpNewReno";
    //defining the starting and ending time foe the CBR traffic agents as given in the question
    double start_time[5] = {0.2, 0.4, 0.6, 0.8, 1.0};
    double end_time[5]   = {1.8, 1.8, 1.2, 1.4, 1.6};

    //user can give input using the command line
    // Use ./ns3 run scratch/lab4 -- --mxbytes=value for entering mxbytes
    // Use ./ns3 run scratch/lab4 -- --tcp_protocol_name=specific_tcp_type" for running with specific tcp type
    cmd.AddValue("mxbytes","Total number of bytes for application to send", mxbytes);
    cmd.AddValue("tcp_protocol_name", "TcpNewReno, TcpHybla, TcpWestwood, TcpScalable, TcpVegas", tcp_protocol_name);
    cmd.Parse(argc, argv);
    string prot_names[5];
    prot_names[0] = "TcpNewReno";
    prot_names[1] = "TcpHybla";
    prot_names[2] = "TcpWestwood";
    prot_names[3] = "TcpScalable";
    prot_names[4] = "TcpVegas";
    int prot_idx = -1;
    int j = 0;
    while(j<5){
        if(tcp_protocol_name == prot_names[j]){
            prot_idx = j;
            break;
        }
        j++;
    }
    if(prot_idx == -1){
        NS_LOG_DEBUG("Tcp type is not valid");
        exit(1);
    }
    switch (prot_idx) {
        case 0:
            Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
            break;
        case 1:
            Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));
            break;
        case 2:
            Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
            break;
        case 3:
            Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));
            break;
        case 4:
            Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));
            break;
        default:
            NS_LOG_DEBUG("Not Valid TCP Type");
            exit(1);
    }

    //Create file streams for data storage
    Ptr<OutputStreamWrapper> dropped_packets = ascii.CreateFileStream(tcp_protocol_name + "_packets_dropped.dat");
    Ptr<OutputStreamWrapper> cwnd_stat = ascii.CreateFileStream(tcp_protocol_name + "_cwnd_stat.dat");
    Ptr<OutputStreamWrapper> bytes_transferred = ascii.CreateFileStream(tcp_protocol_name + "_bytes_transferred.dat");

    //Explicitly create the point-to-point link required by the topology
    NS_LOG_INFO("Creating channels");
    PointToPointHelper point2point;
    //Bandwidth * delay= 10^4 bits = 1250 Bytes and we have set packet size = 1250 Bytes
    point2point.SetQueue("ns3::DropTailQueue","MaxSize", StringValue ("1p")); //1 packet
    // Creating point-to-point link with given Data rate 1Mbps and Delay 10ms
    point2point.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    point2point.SetChannelAttribute("Delay", StringValue("10ms"));
    //Explicitly create the nodes required by the topology
    NS_LOG_INFO("Creating nodes");
    NodeContainer nodes;
    nodes.Create(2);
    //Attaching nodes to the NetDeviceContainer
    NetDeviceContainer devs; //devices
    devs=point2point.Install(nodes);
    //Install internet stack on the nodes
    InternetStackHelper intnet;
    intnet.Install(nodes);
    //creating error model
    Ptr<RateErrorModel> error_model=CreateObject<RateErrorModel> ();
    error_model->SetAttribute("ErrorRate", DoubleValue(0.00001));
    devs.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(error_model));
    //after getting the hardware in place, we now need to add IP Address
    NS_LOG_INFO ("Assigning IP Address");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4Container = ipv4.Assign(devs);
    //Create a BulkSendApplication and install it on node 0
    NS_LOG_INFO("Create Applications");
    uint16_t port = 12100;
    BulkSendHelper src("ns3::TcpSocketFactory",InetSocketAddress(ipv4Container.GetAddress(1), port));
    //Set the amount of data to send in bytes. Zero is unlimited.
    src.SetAttribute("MaxBytes",UintegerValue(mxbytes));
    ApplicationContainer appSrc = src.Install(nodes.Get(0));
    //assigning the time of simulation according to question
    appSrc.Start(Seconds(0.0));
    appSrc.Stop(Seconds(1.80));
    //Create a PacketSinkApplication and install it on node 1
    PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(),port));
    ApplicationContainer appsink = sink.Install(nodes.Get(1));
    appsink.Start(Seconds(0.0));
    appsink.Stop(Seconds(1.80));
    sinkTcp =DynamicCast<PacketSink>(appsink.Get(0));
    //defining a port
    uint16_t cbrPort = 12000;
    int i=0;
    while(i<5){
        //Install applications: five CBR streams each saturating the channel
        ApplicationContainer cbrSinkApps;
        ApplicationContainer cbrApps;
        //to get the different port numbers cbrPort+i is used 
        OnOffHelper onOffHelper("ns3::UdpSocketFactory",InetSocketAddress(ipv4Container.GetAddress(1), cbrPort+i));
        onOffHelper.SetAttribute("DataRate",StringValue("300Kbps"));
        onOffHelper.SetAttribute("StartTime",TimeValue(Seconds(start_time[i])));
        onOffHelper.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onOffHelper.SetAttribute("PacketSize",UintegerValue(1024));
        onOffHelper.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("StopTime",TimeValue(Seconds(end_time[i])));
        cbrApps.Add(onOffHelper.Install(nodes.Get(0)));
        
        //Packet sinks for each CBR agent

        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), cbrPort+i));
        cbrSinkApps = sink.Install(nodes.Get(1));
        cbrSinkApps.Start(Seconds(0.0));
        cbrSinkApps.Stop(Seconds(1.8));
        cbr[i] = DynamicCast<PacketSink>(cbrSinkApps.Get(0));
        i++;
    }
    //Running the simulation
    NS_LOG_INFO("Run Simulation");
    Simulator::Schedule(Seconds(0.00001),&TotalRx,bytes_transferred);
    Simulator::Schedule(Seconds(0.00001),&TraceCwnd,cwnd_stat);
    //flowMontor to display various stats
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(1.80));
    //tracking the dropped packets
    devs.Get(1)->TraceConnectWithoutContext("PhyRxDrop", MakeBoundCallback(&RxDrop, dropped_packets));
    Simulator::Run();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
    FlowMonitor::FlowStats temp = stats[1];

    cout << "Flow monitor statistics for " << tcp_protocol_name <<  " TCP congestion control algorithm\n";
    cout << "Total Transferred Packets  =   " << temp.txPackets << endl;
    cout << "Total Received Packets  =   " << temp.rxPackets << endl;
    cout << "Total Transferred Bytes  =   " << temp.txBytes << endl;
    cout << "Total Received Bytes  =   " << temp.rxBytes << endl;
    cout << "Total Offered Load  =   " << temp.txBytes*8.0/(temp.timeLastTxPacket.GetSeconds()-temp.timeFirstTxPacket.GetSeconds())/1000000<< " Mbps" << endl;
    cout << "Throughput  =   " << temp.rxBytes*8.0/(temp.timeLastRxPacket.GetSeconds()-temp.timeFirstRxPacket.GetSeconds())/1000000<< " Mbps"<< endl;
    
    flowMonitor->SerializeToXmlFile("stats.flowmon", true, true);
    Simulator::Destroy();
    return 0;
}