#include <array>
#include <iostream>
#include <QByteArray>
#include <QDateTime>

#include "CSV.hpp"
#include "../Scratch.hpp"

#define HALT(msg)

enum EColumn
{
    ColumnDate,
    ColumnTime,
    ColumnAreaPixel,
    ColumnAreaUM,
    ColumnRemaining,
    ColumnHeal,
    ColumnWidthAVG,
    ColumnWidthMED,
    ColumnWidthSTD,
    ColumnSpeedArea,
    ColumnSpeedWidth,
    ColumnQuality,
    NumberOfColumns
};

std::array<const char*, NumberOfColumns> columnNameList = {
    "日期",
    "时间(h)",
    "伤口面积(像素)",
    "伤口面积(μm²)",
    "剩余伤口占比(%)",
    "伤口愈合率(%)",
    "伤口宽度平均数(μm)",
    "伤口宽度中值(μm)",
    "伤口宽度标准差",
    "伤口面积闭合速度(μm²/h)",
    "伤口宽度闭合速度(μm/h)",
    "质量标记"
};

static const char* toString(int quality)
{
    switch (quality)
    {
        case ScratchQualityNormal:      return "正常";
        case ScratchQualitySmall:       return "划痕过小";
        case ScratchQualityUneven:      return "划痕不均匀";
        case ScratchQualityAbnormal:    return "划痕异常";
        default: return "INVALID";
    }
}

int CCSVSerializer::process(
    const uint64_t *timestampList, 
    const ScratchResultKinetic &data, 
    size_t size, 
    std::ostream &os)
{
    QString value;
    uint64_t baseTimestamp = timestampList[0];

    for (int columnIndex = 0; columnIndex < columnNameList.size(); ++columnIndex)
        os << columnNameList[columnIndex] << ((columnIndex != (columnNameList.size() - 1)) ? ',' : '\n');

    for (int i = 0; i < size; ++i)
    {
        for (int columnIndex = 0; columnIndex < columnNameList.size(); ++columnIndex)
        {
            switch(columnIndex)
            {
                case ColumnDate:
                {
                    os << QDateTime::fromSecsSinceEpoch(timestampList[i]).toString("yyyy-MM-dd hh-mm-ss").toUtf8().constData() << ',';
                    break;
                }
                case ColumnTime:
                {
                    auto time = (timestampList[i] - baseTimestamp) / 3600.0;
                    os << QByteArray::number(time, 'f', 4).constData() << ',';
                    break;
                }
                case ColumnAreaPixel:
                {
                    os << QByteArray::number(data.frames[i].raw.area.pixel, 'f', 0).constData() << ',';
                    break;
                }
                case ColumnAreaUM:
                {
                    os << QByteArray::number(data.frames[i].raw.area.um, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnRemaining:
                {
                    os << QByteArray::number((1-data.frames[i].heal.corrected)*100, 'f', 0).constData() << ',';
                    break;
                }
                case ColumnHeal:
                {
                    os << QByteArray::number(data.frames[i].heal.corrected*100, 'f', 0).constData() << ',';
                    break;
                }
                case ColumnWidthAVG:
                {
                    os << QByteArray::number(data.frames[i].raw.width.avg, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnWidthMED:
                {
                    os << QByteArray::number(data.frames[i].raw.width.med, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnWidthSTD:
                {
                    os << QByteArray::number(data.frames[i].raw.width.std, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnSpeedArea:
                {
                    os << QByteArray::number(data.frames[i].speed.area, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnSpeedWidth:
                {
                    os << QByteArray::number(data.frames[i].speed.width, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnQuality:
                {
                    os << toString(data.frames[i].quality) << '\n';
                    break;
                }
                default:
                    HALT("Out Of Range");
            }
        }
    }
    
    return 0;
}
