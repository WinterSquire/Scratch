#include <QJsonArray>

#include "JSON.hpp"
#include "../Scratch.hpp"

int CJSONSerializer::process(
    const uint64_t *timestampList, 
    const ScratchResultKinetic &data, 
    size_t size, 
    std::ostream &os)
{
    QJsonObject root;
    QJsonArray frames, timestamps;

    for (int i = 0; i < size; ++i)
    {
        timestamps.push_back((qint64)timestampList[i]);
        frames.push_back(toJson(data.frames[i]));
    }

    root["t50"] = data.t50;
    root["t90"] = data.t90;
    root["frames"] = frames;
    root["timestamps"] = timestamps;

    os << QJsonDocument(root).toJson().constData();

    return 0;
}

QJsonObject toJson(const ScratchParameter& data)
{
    return QJsonObject::fromVariantMap({
        {"fillHole", data.fillHole},
        {"dx", data.dx},
        {"dy", data.dy},
    });
}

int fromJson(ScratchParameter& data, const QJsonObject& json)
{
    data.fillHole = json.value("fillHole").toInt();
    data.dx = json.value("dx").toDouble();
    data.dy = json.value("dy").toDouble();
    
    return 0;
}

QJsonObject toJson(const ScratchArea& data)
{
    return QJsonObject::fromVariantMap({
        {"pixel", data.pixel},
        {"um", data.um}
    });
}

int fromJson(ScratchArea& data, const QJsonObject& json)
{
    data.pixel = json.value("pixel").toDouble();
    data.um = json.value("um").toDouble();
    return 0;
}

QJsonObject toJson(const ScratchInvasionData& data)
{
    return QJsonObject::fromVariantMap({
        {"area", toJson(data.area)},
        {"ratio", data.ratio}
    });
}

int fromJson(ScratchInvasionData& data, const QJsonObject& json)
{
    fromJson(data.area, json.value("area").toObject());
    data.ratio = json.value("ratio").toDouble();
    return 0;
}

QJsonObject toJson(const ScratchResult& data)
{
    return QJsonObject::fromVariantMap({
        {"area", toJson(data.area)},
        {"width", QJsonObject::fromVariantMap({
            {"avg", data.width.avg},
            {"std", data.width.std},
            {"med", data.width.med},
        })},
        {"roughness", QJsonObject::fromVariantMap({
            {"left", data.roughness.left},
            {"right", data.roughness.right}
        })},
        {"confluence", data.confluence}
    });
}

int fromJson(ScratchResult& data, const QJsonObject& json)
{
    fromJson(data.area, json.value("area").toObject());
    const auto width = json.value("width").toObject();
    data.width.avg = width.value("avg").toDouble();
    data.width.std = width.value("std").toDouble();
    data.width.med = width.value("med").toDouble();
    const auto roughness = json.value("roughness").toObject();
    data.roughness.left = roughness.value("left").toDouble();
    data.roughness.right = roughness.value("right").toDouble();
    data.confluence = json.value("confluence").toDouble();
    return 0;
}

QJsonObject toJson(const ScratchResultFrame& data)
{
    return QJsonObject::fromVariantMap({
        {"raw", toJson(data.raw)},
        {"heal", QJsonObject::fromVariantMap({
            {"raw", data.heal.raw},
            {"corrected", data.heal.corrected}
        })},
        {"speed", QJsonObject::fromVariantMap({
            {"area", data.speed.area},
            {"width", data.speed.width}
        })},
        {"quality", data.quality}
    });
}

int fromJson(ScratchResultFrame& data, const QJsonObject& json)
{
    fromJson(data.raw, json.value("raw").toObject());
    const auto heal = json.value("heal").toObject();
    const auto speed = json.value("speed").toObject();
    data.heal.raw = heal.value("raw").toDouble();
    data.heal.corrected = heal.value("corrected").toDouble();
    data.speed.area = speed.value("area").toDouble();
    data.speed.width = speed.value("width").toDouble();
    data.quality = json.value("quality").toInt();
    return 0;
}