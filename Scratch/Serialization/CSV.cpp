#include <array>
#include <iostream>
#include <QByteArray>
#include <QDateTime>

#include "CSV.hpp"
#include "../Scratch.hpp"

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

int CCSVSerializer::serialize(size_t size, const ScratchParameterKinetic *exp, const ScratchParameterKinetic *con, const ScratchParameterGlobal &parameter, std::ostream &os)
{
    QString value;
    uint64_t baseTimestamp = exp->timestamps[0];

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
                    os << QDateTime::fromSecsSinceEpoch(exp->timestamps[i]).toString("yyyy-MM-dd hh-mm-ss").toUtf8().constData() << ',';
                    break;
                }
                case ColumnTime:
                {
                    auto time = (exp->timestamps[i] - baseTimestamp) / 3600.0;
                    os << QByteArray::number(time, 'f', 4).constData() << ',';
                    break;
                }
                case ColumnAreaPixel:
                {
                    os << QByteArray::number(exp->frames[i].scratchArea.pixel, 'f', 0).constData() << ',';
                    break;
                }
                case ColumnAreaUM:
                {
                    os << QByteArray::number(exp->frames[i].scratchArea.um, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnRemaining:
                {
                    os << QByteArray::number((1-exp->frames[i].heal)*100, 'f', 0).constData() << ',';
                    break;
                }
                case ColumnHeal:
                {
                    os << QByteArray::number(exp->frames[i].heal*100, 'f', 0).constData() << ',';
                    break;
                }
                case ColumnWidthAVG:
                {
                    os << QByteArray::number(exp->frames[i].width.avg, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnWidthMED:
                {
                    os << QByteArray::number(exp->frames[i].width.med, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnWidthSTD:
                {
                    os << QByteArray::number(exp->frames[i].width.std, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnSpeedArea:
                {
                    os << QByteArray::number(exp->frames[i].speed.area, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnSpeedWidth:
                {
                    os << QByteArray::number(exp->frames[i].speed.width, 'f', 2).constData() << ',';
                    break;
                }
                case ColumnQuality:
                {
                    os << toString(exp->frames[i].quality) << '\n';
                    break;
                }
                default:
                    HALT("Out Of Range");
            }
        }
    }
    
    return 0;
}
