#include <dataprocessthread.h>

DataProcessThread::DataProcessThread(const QJsonObject &rsulocation)
{
//    queueVehicleOne = new QQueue<QJsonObject>();
//    queueVehicleTwo = new QQueue<QJsonObject>();
    this->rsuLocation = rsulocation;
    idOne = 0;
    idTwo = 0;
}

DataProcessThread::~DataProcessThread(){
    if(timer != nullptr){
        disconnect(timer, &QTimer::timeout, this, &DataProcessThread::timeOutSlot);
        this->timer->stop();
        this->timer->destroyed();
        this->timer = nullptr;
    }
}

/**
 * 重载run函数，线程运行的入口
 * @brief DataProcessThread::run
 */
void DataProcessThread::run(){
    timer = new QTimer();
    timer->setInterval(FREQUENCY);
    connect(timer, &QTimer::timeout, this, &DataProcessThread::timeOutSlot);
    timer->start();
    this->exec();
}


/**
 * 向队列中添加Message
 * @brief DataProcessThread::addMessage
 * @param message
 * @return
 */
bool DataProcessThread::addMessage(const QJsonObject &message){
    bool isSuccess = true;
    int sizeOne = queueVehicleOne.size();
    int sizeTwo = queueVehicleTwo.size();
    int idMessage = message.find("id").value().toInt();
    if(0 == sizeOne && 0 == sizeTwo){    //两个队列均为空
        idOne = idMessage;
        queueVehicleOne.enqueue(message); //在队尾添加一个元素
    } else if(0 != sizeOne && 0 == sizeTwo){  //第二个队列为空
        if(idMessage != idOne){
            idTwo = idMessage;
            queueVehicleTwo.enqueue(message);
        }else{
            queueVehicleOne.enqueue(message);
        }
    } else if(0 != sizeTwo && 0 == sizeOne){   //第一个队列为空
        if(idMessage != idTwo){
            idOne = idMessage;
            queueVehicleOne.enqueue(message);
        }else{
            queueVehicleTwo.enqueue(message);
        }
    } else{                                  //两个队列均不为空
        if(idMessage == idOne){
            queueVehicleOne.enqueue(message);
        } else if(idMessage == idTwo){
            queueVehicleTwo.enqueue(message);
        } else{
            isSuccess = false;
        }
    }
    return isSuccess;
}

/**
 * 返回是否可以计算, 如果可以，返回包含四个QJsonObject的List
 * @brief DataProcessThread::isComputed
 * @return
 */
QList<QJsonObject> DataProcessThread::isComputed(){
    QList<QJsonObject> list;
    int sizeOne = queueVehicleOne.size();
    int sizeTwo = queueVehicleTwo.size();
    if(sizeOne >= 2 && sizeTwo >= 2){
        //车辆一的队列出队两个
        list.append(queueVehicleOne.dequeue());
        list.append(queueVehicleOne.dequeue());
        //车辆二的队列出队两个
        list.append(queueVehicleTwo.dequeue());
        list.append(queueVehicleTwo.dequeue());
    }
    return list;
}

QList<QList<double>> Trajectory(double t, double v, double a, double rlat, double rlon, double vlat, double vlon){
    double l,h,d;//小三角形斜边长,高，底边长
    double L,H,D;//大三角形斜边长，高，底边长
    const double lat_THRESHOLD = 0.001;//经度的精度
    const double lon_THRESHOLD = 0.001;//纬度的精度
    QList<double> array;
    QList<QList<double>> Tra;
    L=sqrt(pow(rlat-vlat,2)+pow(rlon-vlon,2));
    H=rlon-vlon;
    D=rlat-vlat;
    int t0=t;
    while (abs(rlat-vlat)>lat_THRESHOLD && abs(rlon-vlon)>lon_THRESHOLD) {
        l=v*t+1/2*a*pow(t,2);    //计算斜边长
        h=l/L*H;                 //计算高
        d=l/L*D;                 //计算底边
        vlat=vlat+d;             //计算下t时间后的经度
        vlon=vlon+h;             //计算下t时间后的纬度
        //将当前经纬度保存进QList
        array[0]=vlat;
        array[1]=vlon;
        //保存轨迹
        Tra.append(array);
        //更新时间，求下一时间点的经纬度
        t=t+t0;
    }
    return Tra;
}
/**
 * 根据List计算结果
 * @brief DataProcessThread::ComputerResult
 * @param list
 */
void DataProcessThread::ComputerResult(const QList<QJsonObject> &list){
    QList<QJsonObject> result;
    QJsonObject RVehicleOne,RVehicleTwo;
    QList<QList<double>> tra1,tra2;
    //提取两辆车的id
    int id1=list.at(0).find("id").value().toString().toInt();
    int id2=list.at(2).find("id").value().toString().toInt();

    //提取两辆车的速度
    float v11=list.at(0).find("speed").value().toString().toFloat();
    float v12=list.at(1).find("speed").value().toString().toFloat();
    float v21=list.at(2).find("speed").value().toString().toFloat();
    float v22=list.at(3).find("speed").value().toString().toFloat();
    //求两辆车的平均速度
    float v1=(v11+v12)/2;
    float v2=(v21+v22)/2;

    //提取两辆车的加速度
    double acc11=list.at(0).find("acc").value().toString().toDouble();
    double acc12=list.at(1).find("acc").value().toString().toDouble();
    double acc21=list.at(2).find("acc").value().toString().toDouble();
    double acc22=list.at(3).find("acc").value().toString().toDouble();
    //求两辆车的平均加速度
    double acc1=(acc11+acc12)/2;
    double acc2=(acc21+acc22)/2;

    //提取两辆车的lat
    double lat11=list.at(0).find("lat").value().toString().toDouble();
    double lat12=list.at(1).find("lat").value().toString().toDouble();
    double lat21=list.at(2).find("lat").value().toString().toDouble();
    double lat22=list.at(3).find("lat").value().toString().toDouble();
    //求两辆车的平均lat（NTU)
    double lat1=(lat11+lat12)/2;
    double lat1_NTU=lat1*100000;
    double lat2=(lat21+lat22)/2;
    double lat2_NTU=lat2*100000;

    //提取两辆车的lon
    double lon11=list.at(0).find("lon").value().toString().toDouble();
    double lon12=list.at(1).find("lon").value().toString().toDouble();
    double lon21=list.at(2).find("lon").value().toString().toDouble();
    double lon22=list.at(3).find("lon").value().toString().toDouble();
    //求两辆车的平均lon（NTU)
    double lon1=(lon11+lon12)/2;
    double lon1_NTU=lon1*100000;
    double lon2=(lon21+lon22)/2;
    double lon2_NTU=lon2*100000;

    //提取RSU的lat(NTU)
    double Rlat=list.at(4).find("lat").value().toString().toDouble();
    double Rlat_NTU=Rlat*100000;

    //提取RSU的lon(NTU)
    double Rlon=list.at(4).find("lon").value().toString().toDouble();
    double Rlon_NTU=Rlon*100000;

    //求两车与路口的距离
    double dist1=sqrt(pow(lat1_NTU-Rlat_NTU,2)+pow(lon1_NTU-Rlon_NTU,2));
    double dist2=sqrt(pow(lat2_NTU-Rlat_NTU,2)+pow(lon2_NTU-Rlon_NTU,2));

    //求两车可能的碰撞时间
    double t1=(-v1+sqrt(pow(v1,2)+2*acc1*dist1))/acc1;
    double t2=(-v2+sqrt(pow(v2,2)+2*acc2*dist2))/acc2;

    if (abs(t1-t2)<THRESHOLD){//碰撞
        RVehicleOne.insert("id",id1);
        RVehicleOne.insert("warning",true);
        RVehicleOne.insert("time",t1);
        RVehicleOne.insert("distance",dist1);
        //求车1的碰撞轨迹
        tra1=Trajectory(UNITTIME,v1,acc1,Rlat,Rlon,lat1,lon1);
        //RVehicleOne.insert("trajectory",tra1);  //轨迹insert不进去

        RVehicleTwo.insert("id",id2);
        RVehicleTwo.insert("warning",true);
        RVehicleTwo.insert("time",t2);
        RVehicleTwo.insert("distance",dist2);
        //求碰撞轨迹
        tra2=Trajectory(UNITTIME,v2,acc2,Rlat,Rlon,lat2,lon2);
        //RVehicleTwo.insert("trajectory",tra2);  //轨迹insert不进去
    }else{//安全
        RVehicleOne.insert("id",id1);
        RVehicleOne.insert("warning",false);
        RVehicleOne.insert("distance",dist1);

        RVehicleTwo.insert("id",id2);
        RVehicleTwo.insert("warning",false);
        RVehicleTwo.insert("distance",dist2);
    }
   //将结果保存进result

    emit sendResult(RVehicleOne);
   // emit sendResult(RVehicleTwo);

    result.append(RVehicleOne);
    result.append(RVehicleTwo);

}

void DataProcessThread::timeOutSlot(){
    QList<QJsonObject> list = isComputed();
    if(0 != list.size()){
        ComputerResult(list);
    }
}

