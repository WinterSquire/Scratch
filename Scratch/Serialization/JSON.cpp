#include <QJsonArray>

#include "JSON.hpp"
#include "../Scratch.hpp"

int CJSONSerializer::process(
    const uint64_t* timestampList, 
    const struct ScratchResultFrame* frames,
    size_t size, 
    const struct ScratchResultKinetic& data, 
    std::ostream& os)
{
    QJsonObject jRoot;
    QJsonArray jFrames, jTimestamps;

    for (int i = 0; i < size; ++i)
    {
        jTimestamps.push_back((qint64)timestampList[i]);
        jFrames.push_back(toJson(frames[i]));
    }

    jRoot["t50"] = data.t50;
    jRoot["t90"] = data.t90;
    jRoot["frames"] = jFrames;
    jRoot["timestamps"] = jTimestamps;

    os << QJsonDocument(jRoot).toJson().constData();

    return 0;
}

QJsonObject toJson(const ScratchParameter& data)
{
    return QJsonObject::fromVariantMap({
        {"dx", data.dx},
        {"dy", data.dy},
    });
}

int fromJson(ScratchParameter& data, const QJsonObject& json)
{
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

QJsonObject toJson(const ScratchResult& data)
{
    return QJsonObject::fromVariantMap({
        {"scratchArea", toJson(data.scratchArea)},
        {"invasionArea", toJson(data.invasionArea)},
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
    fromJson(data.scratchArea, json.value("scratchArea").toObject());
    fromJson(data.invasionArea, json.value("invasionArea").toObject());
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
    auto&& jObject = toJson(*(ScratchResult*)&data);

    jObject["heal"] = data.heal;
    jObject["quality"] = data.quality;
    jObject["speed"] = QJsonObject::fromVariantMap({
        {"area", data.speed.area},
        {"width", data.speed.width}
    });

    return jObject;
}

int fromJson(ScratchResultFrame& data, const QJsonObject& json)
{
    fromJson(*(ScratchResult*)&data, json);

    data.heal = json.value("heal").toDouble();
    data.quality = json.value("quality").toInt();

    const auto speed = json.value("speed").toObject();
    data.speed.area = speed.value("area").toDouble();
    data.speed.width = speed.value("width").toDouble();

    return 0;
}