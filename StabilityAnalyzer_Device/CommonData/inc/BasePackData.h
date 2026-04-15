#ifndef BASEPACKDATA_H
#define BASEPACKDATA_H

#include <QObject>
#include <QVector>
#include <QList>
#include <QMetaEnum>
#include <QMap>

// 转字符串
#define STRS(s) #s
#define TOSTR(args) STRS(args)

// 自定义数据段属性名称 QObject设置与读取
#define SUBJECT             "subject"                      //专业
#define SUBJECT_CHILD       "subject_child"                //专业子专业(轮轨力分为特征数据 平稳性数据)
#define DATA                "data"                         //专业数据
#define TIME_ORIGINAL       "time_original"                //原始采集时间

// 专业类型
// [https://forum.qt.io/topic/59869/q_enum-enum_name-works-with-only-enums-declared-inside-class-srsly/5]
// namespace + Q_ENUM_NS 将enum枚举与字符串转换
namespace SubjectType
{
Q_NAMESPACE

enum EM_Subject
{
    None ,              // 无                   0
    WheelForce,         // 轮轨力(车辆动力学)     1
    WheelGeometry,      // 轨道几何              2
    Pantograp,          // 弓网                 3
    Contour,            // 廓形                 4
    TunnelSpect,        // 隧道综合巡检          5
    LineSpect,          // 线路巡检              6
    FourC,              // 4C                  7
    WearTear,           // 磨耗                 8
    TimeLocation,       // 占位，未使用          9
    TrackSpect,         // 轨道巡检              10
    Communicate,        // 通信                 11
    Signal,             // 信号                 12
    Wayside=13,         // 轨旁专业             13
    RunningGear=14,     // 走行部              14

};
// 使用Q_ENUMS 无法使用metaEnum的valueTokey方法
Q_ENUM_NS(EM_Subject)

/**
 * @brief The SubjectEnumOperate class
 * 获取专业枚举遍历值
 * 专业列表数据库初始化 由专业枚举确定
 */
class SubjectEnumOperate : public QObject
{
public:
    explicit SubjectEnumOperate(){
        m_subjectIdToNameMap.clear();
        // 缓存专业id与名称
        QMetaEnum metaEnum = QMetaEnum::fromType<EM_Subject>();
        for (int i = 0; i < metaEnum.keyCount(); i++) {
            if(None == metaEnum.value(i)){
                continue;
            }
            int s = metaEnum.value(i);
            QString subjectName = tr("未知专业");
            switch(s){
            case WheelGeometry:
                subjectName = tr("轨道几何");
                break;
            case WheelForce:
                subjectName = tr("车辆动力学");
                break;
            case Pantograp:
                subjectName = tr("弓网");
                break;
            case Contour:
                subjectName = tr("廓形");
                break;
            case TunnelSpect:    // 旧的 case TunneSpect:
                subjectName = tr("轨道巡检");
                break;
            case LineSpect:
                subjectName = tr("隧道综合巡检");
                break;
            case FourC:
                subjectName = tr("4C");
                break;
            case WearTear:
                subjectName = tr("磨耗");
                break;
            case Communicate:
                subjectName = tr("通信");
                break;
            case Signal:
                subjectName = tr("信号");
                break;
            case RunningGear:
                subjectName = tr("走行部");
                break;
            case Wayside:
                subjectName=tr("轨旁");
                break;
             case TrackSpect:
                subjectName=tr("轨道巡检");
                break;
            }
            m_subjectIdToNameMap[s] = subjectName;
        }
    }
    static SubjectEnumOperate* instance(){
        static SubjectEnumOperate* s = nullptr;
        if(s == nullptr){
            s = new SubjectEnumOperate;
        }
        return s;
    }
    /// 获取专业枚举列表 包括所有专业
    static QList<int> subjectEnum_All(){
        QMetaEnum metaEnum = QMetaEnum::fromType<EM_Subject>();
        QList<int> list;
        for (int i = 0; i < metaEnum.keyCount(); i++) {
            //list += metaEnum.valueToKey(metaEnum.value(i));
            if(None == metaEnum.value(i)){
                continue;
            }
            list.append(metaEnum.value(i));
        }
        return list;
    }
    /// 获取专业枚举列表 指定不包括某个专业
    static QList<int> subjectEnum_WithoutOne(int subject){
        QMetaEnum metaEnum = QMetaEnum::fromType<EM_Subject>();
        QList<int> list;
        for (int i = 0; i < metaEnum.keyCount(); i++) {
            if(subject == metaEnum.value(i) || None == metaEnum.value(i)){
                continue;
            }
            list.append(metaEnum.value(i));
        }
        return list;
    }
    /// 查询专业名称
    QString subjectName(const int& subject){
        if(m_subjectIdToNameMap.contains(subject)){
            return m_subjectIdToNameMap[subject];
        }
        return "";
    }
private:
    QMap<int, QString> m_subjectIdToNameMap;
};
}

/**
 * @brief The Transform class
 * 转码 放在这里没有放在通用方法模块 是为了减少对其他模块的依赖
 */
class Transform : public QObject
{
    Q_OBJECT
public:
    explicit Transform(QObject* parent = nullptr){
        Q_UNUSED(parent)
    }
    /* 大小端转换 */
    static void BLEndianUint64(double *value){
        unsigned int* val = (unsigned int *)value;

        BLEndianUint32(val);
        BLEndianUint32(val+1);

        unsigned int temp = val[0];
        val[0] = val[1];
        val[1] = temp;
    }
    static void BLEndianUint64(uint64_t *value){
        unsigned int* val = (unsigned int *)value;

        BLEndianUint32(val);
        BLEndianUint32(val+1);

        unsigned int temp = val[0];
        val[0] = val[1];
        val[1] = temp;
    }
    static void BLEndianUint32(unsigned int *value){
        unsigned int val = *value;
        *value = ((val & 0x000000FF) << 24) |  ((val & 0x0000FF00) << 8) |  ((val & 0x00FF0000) >> 8) | ((val & 0xFF000000) >> 24);

    }
    static void BLEndianUint16(uint16_t *value){
        uint16_t val = *value;
        *value = ((val & 0x00FF)<<8)|((val & 0xFF00)>>8);

    }

    /* 逆序memcpy */
    static void reversememcpy(void * des, const void * src, size_t len){
        /* 4个字节copy，减少循环次数 */
        size_t size = len / 4;
        size_t mod = len % 4;

        char *destemp = (char *)des + len;
        char *srctemp = (char *)src;

        while (size--)
        {
            *--destemp = *srctemp++;
            *--destemp = *srctemp++;
            *--destemp = *srctemp++;
            *--destemp = *srctemp++;
        }
        for (size_t i = 0; i < mod; i++) {
            *--destemp = *srctemp++;
        }
    }
    /**
     * @brief GetBitChar 获取int8值中某个二进制位的值
     * @param value uint8_t值
     * @param bitIndex 二进制所在的位数
     * @return 字符
     */
    static char GetBitChar(uint8_t value,int bitIndex){
        char ty=((value>>(8-bitIndex))&1)+'0';
        return ty;
    }
    /**
     * @brief GetChars 将int8值的8位二进制位保存为字节数组
     * @param value int8数值
     * @param charsValue 字符数组
     */
    static void GetChars(uint8_t value,char* charsValue)
    {
        for (int i = 7; i >= 0; i--) {
            charsValue[i] = (value & 1) + '0'; // 提取最低位并转换为字符型
            value >>= 1; // 将value向右位移一位，准备提取下一位
        }
    }
    /**
     * @brief GetChars 将int32值的32位二进制位保存为字节数组
     * @param value int32数值
     * @param charsValue 字符数组
     */
    static void getChars2(uint32_t value, char* charsValue)
    {
        for (int i = 31; i >= 0; i--) {
            charsValue[i] = (value & 1) + '0'; // 提取最低位并转换为字符型
            value >>= 1; // 将value向右位移一位，准备提取下一位
        }
    }
};

#endif // BASEPACKDATA_H
