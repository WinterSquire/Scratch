#include <QJsonArray>
#include <opencv2/imgcodecs.hpp>
#include <ostream>
#include <vector>

#include "../Scratch.hpp"
#include "JSON.hpp"

int CDataJsonSerializer::serialize(size_t size, const ScratchParameterKinetic *exp, const ScratchParameterKinetic *con, const ScratchParameterGlobal &parameter, std::ostream &os)
{
    QJsonObject jRoot;

    if (exp) jRoot["exp"] = toJson(*exp, size);
    if (con) jRoot["con"] = toJson(*con, size);
    
    jRoot["parameter"] = toJson(parameter);

    os << QJsonDocument(jRoot).toJson(QJsonDocument::Compact).constData();
    return 0;
}

int CImagesJsonSerializer::serialize(size_t size, const ScratchParameterKinetic *exp, const ScratchParameterKinetic *con, const ScratchParameterGlobal &parameter, std::ostream &os)
{
    QJsonObject jRoot;
    QJsonArray imageRaw, imageMask, imageContour;

    if (exp->images)
        for (size_t i = 0; i < size; ++i)
            imageRaw.append(cvMatToBase64(exp->images[i]).constData());

    if (exp->debugImages && exp->debugImages[ScratchAnalyseStageMasking])
        for (size_t i = 0; i < size; ++i)
            imageMask.append(cvMatToBase64(exp->debugImages[ScratchAnalyseStageMasking][i]).constData());
    
    if (exp->debugImages && exp->debugImages[ScratchAnalyseStageContouring])
        for (size_t i = 0; i < size; ++i)
            imageContour.append(cvMatToBase64(exp->debugImages[ScratchAnalyseStageContouring][i]).constData());

    jRoot["imageRaw"] = imageRaw;
    jRoot["imageMask"] = imageMask;
    jRoot["imageContour"] = imageContour;

    os << QJsonDocument(jRoot).toJson(QJsonDocument::Compact).constData();
    return 0;
}

QByteArray CImagesJsonSerializer::cvMatToBase64(const cv::Mat &mat)
{
    if (mat.empty())
        return {};

    std::vector<uchar> encoded;
    if (!cv::imencode(".jpg", mat, encoded))
        return {};

    const QByteArray base64 = QByteArray(
        reinterpret_cast<const char *>(encoded.data()),
        static_cast<qsizetype>(encoded.size())).toBase64();

    return "data:image/jpg;base64," + base64;
}

QJsonObject toJson(const struct ScratchParameterGlobal& data)
{
    auto procedure = [](const auto& value) {
        return QJsonObject::fromVariantMap({
            {"method", value.method},
            {"data", QJsonObject()} // todo
        });
    };

    return QJsonObject::fromVariantMap({
        {"flags", data.flags},
        {"dx", data.dx},
        {"dy", data.dy},
        {"masking", procedure(data.masking)},
        {"contouring", procedure(data.contouring)}
    });
}

int fromJson(struct ScratchParameterGlobal& data, const QJsonObject& json)
{
    data.flags = json.value("flags").toInt();
    data.dx = json.value("dx").toDouble();
    data.dy = json.value("dy").toDouble();

    // todo: fix
    const auto masking = json.value("masking");
    const auto contouring = json.value("contouring");
    if (masking.isNull() == false)
        data.masking.method = masking.toInt();
    if (contouring.isNull() == false)
        data.contouring.method = contouring.toInt();

    if (json.contains("partition"))
    {
        const auto partition = json.value("partition").toObject();
        if (partition.contains("size"))
            data.partition.size = static_cast<uint64_t>(partition.value("size").toDouble());
    }

    return 0;
}

QJsonObject toJson(const struct ScratchParameterKinetic& data, size_t size)
{
    QJsonObject json;
    json["p"] = data.p;
    json["t50"] = data.t50;
    json["t90"] = data.t90;

    QJsonArray timestamps;
    if (data.timestamps)
        for (size_t i = 0; i < size; ++i)
            timestamps.append(static_cast<qint64>(data.timestamps[i]));
    json["timestamps"] = timestamps;

    QJsonArray frames;
    if (data.frames)
        for (size_t i = 0; i < size; ++i)
            frames.append(toJson(data.frames[i]));
    json["frames"] = frames;

    return json;
}

int fromJson(struct ScratchParameterKinetic& data, const QJsonObject& json)
{
    data.p = json.value("p").toDouble();
    data.t50 = json.value("t50").toDouble();
    data.t90 = json.value("t90").toDouble();

    if (data.timestamps && json.contains("timestamps"))
    {
        const auto values = json.value("timestamps").toArray();
        for (qsizetype i = 0; i < values.size(); ++i)
            data.timestamps[i] = static_cast<uint64_t>(values[i].toVariant().toULongLong());
    }

    if (data.frames && json.contains("frames"))
    {
        const auto values = json.value("frames").toArray();
        for (qsizetype i = 0; i < values.size(); ++i)
            fromJson(data.frames[i], values[i].toObject());
    }

    return 0;
}

QJsonObject toJson(const struct ScratchResult& data)
{
    auto area = [](const auto& value) -> QJsonObject {
        return QJsonObject::fromVariantMap({
            {"pixel", value.pixel},
            {"um", value.um},
        });
    };

    auto stats = [](const auto& value) -> QJsonObject {
        return QJsonObject::fromVariantMap({
            {"avg", value.avg},
            {"std", value.std},
            {"med", value.med},
        });
    };

    auto roughness = [](const auto& value) -> QJsonObject {
        return QJsonObject::fromVariantMap({
            {"left", value.left},
            {"right", value.right},
        });
    };

    auto speed = [](const auto& value) -> QJsonObject {
        return QJsonObject::fromVariantMap({
            {"width", value.width},
            {"area", value.area},
        });
    };


    QJsonObject json;
    json["quality"] = data.quality;
    json["heal"] = data.heal;
    json["confluence"] = data.confluence;

    json["scratchArea"] = area(data.scratchArea);
    json["invasionArea"] = area(data.invasionArea);

    json["width"] = stats(data.width);
    json["roughness"] = roughness(data.roughness);
    json["speed"] = speed(data.speed);

    return json;
}

int fromJson(struct ScratchResult& data, const QJsonObject& json)
{
    const auto scratchArea = json.value("scratchArea").toObject();
    data.scratchArea.pixel = scratchArea.value("pixel").toDouble();
    data.scratchArea.um = scratchArea.value("um").toDouble();

    const auto invasionArea = json.value("invasionArea").toObject();
    data.invasionArea.pixel = invasionArea.value("pixel").toDouble();
    data.invasionArea.um = invasionArea.value("um").toDouble();

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
