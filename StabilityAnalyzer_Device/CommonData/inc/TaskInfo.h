#ifndef TASKINFO_H
#define TASKINFO_H
#include <QObject>

#define SPEED_SYMBOL  90000
#define FILE_HEADER_LEN 8092

enum E_ChainType {
    CHAINTYPE_LONG = 0			// 长链
    , CHAINTYPE_SHORT			// 短链
    , CHAINTYPE_UNCERTAIN		// 不确定
};
enum TaskState
{
    TS_NOSTART = 0,//未开始
    TS_GOING,//进行中
    TS_DONE,//已完成
    TS_STOP, // 已终止
};
enum DataState
{
    DS_NORMAL = 0,//正常
    DS_DISCARD, //废弃
};

// 任务信息
struct TaskInfo
{
    QString id;                 // 任务ID 数据库类型为bigint 8字节
    QString taskName;           // 任务名称
    QString lineId;           // 检测线路id
    QString lineName;           // 检测线路
    QString testStart;          // 测试站点起点
    QString testEnd;            // 测试站点终点
    int speedLevel;            // 速度级
    QString testdate;           // 测试日期
    QString taskState;              // 任务状态
    QString dateState;              // 数据状态
    QString startTime;          //任务开始时间，当状态为0时，点击启动时间
    QString stopTime;           //任务完成时间
    double startMileage;        //开始站里程
    double endMileage;        //结束站里程
    void init()
    {
        id = "";
        taskName = "";
        lineId = "";
        lineName = "";
        testStart = "";
        testEnd = "";
        speedLevel = -1;
        testdate = "";
        taskState = "";
        dateState = "";
        startTime = "";
        stopTime = "";
        startMileage = 0;
        endMileage = 0;
    }
};
Q_DECLARE_METATYPE(TaskInfo)


// 子任务信息
struct SubTaskInfo
{
    QString id;                     // 子任务id
    QString taskId;                 // 任务ID
    QString taskName;           // 任务名称
    QString lineName;           // 检测线路
    QString testStart;          // 测试站点起点
    QString testEnd;            // 测试站点终点

    int directID;               //行別ID 0：上行，1：下行
    int directionID;            //方向ID 0:正向，1：反向
    QString directType;			// 行别
    QString taskDirection;		// 方向
    QString startStation;		// 检测起始车站
    QString endStation;         // 检测终止车站
    double startMile;           // 检测起始里程
    double endMile;             // 检测结束里程
    double centerMile;          // 车站中心里程
    double speedMax;			// 最高试验速度
    QString project;			// 检测项目
    QString projectID;          // 检测项目ID
    QString timeStart;			// 检测开始时间
    QString timeEnd;			// 检测结束时间
    QString state;              // 状态
    void init()
    {
        id = "";
        taskId = "";
        taskName = "";
        lineName = "";
        testStart = "";
        testEnd = "";
        directID = 0;
        directionID =0;
        directType = "";
        taskDirection = "";
        startStation = "";
        endStation = "";
        startMile = 0;
        endMile = 0;
        centerMile = 0;
        speedMax = 0;
        project = "";
        timeStart = "";
        timeEnd = "";
        state = "";
        projectID = "";
    }
};

// 圆曲线信息
struct HorizCurveModel
{
    QString directType;			// 行别
    QString curveNumber;		// 曲线编号
    QString straSlowPoint;		// 直缓点
    QString slowDotPoint;		// 缓圆点
    QString dotSlowPoint;		// 圆缓点
    QString slowStraPoint;		// 缓直点
    double curveRadius;			// 曲线半径(m)
    QString turnDirect;			// 转向
    int overHeight;				// 超高(mm)
    double curveVector;			// 圆曲线正矢(mm)
    double curveLength;			// 曲线全长(m)

};

// 道岔信息
struct SwitchModel
{
    QString lineType;			// 线别
    QString stationName;		// 车站名
    QString directType;			// 行别
    QString switchNumber;		// 道岔编号
    double crossingMileValue;	// 岔心里程值
    QString lineNumber;			// 线路编号

};

// 车站信息
struct StationModel
{
    QString lineType;			// 线别
    QString stationName;		// 车站名
    QString type;				// 类型
    double startMile;			// 起点里程
    double centerMile;			// 中点里程
    double endMile;				// 终止里程
    int switchAmount;			// 道岔数量
    QString lineNumber;			// 线路编号

};

// 长短链
struct LongShortChain
{
    QString lineType;		// 线别
    QString directType;		// 行别
    E_ChainType chainType;		// 断链类型
    double chainMileBefore;	// 断链前里程
    double chainMileAfter;	// 断链后里程
    double chainDistance;	// 断链长度
    QString lineNumber;		// 线路编号
};

#endif // TASKINFO_H
