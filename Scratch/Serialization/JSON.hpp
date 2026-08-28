#pragma once

#include <QJsonDocument>
#include <QJsonObject>

namespace cv { class Mat; }

class CJSONSerializer
{
public:
    static int process(
        const uint64_t* timestampList, 
        const struct ScratchResultFrame* frames,
        size_t size, 
        const struct ScratchResultKinetic& data, 
        std::ostream& os);

    static int process(
        const cv::Mat* images,
        const cv::Mat* debugImages[NumberOfScratchAnalyseStage],
        size_t size, 
        std::ostream& os);
};

#define toJsonString(data) (QJsonDocument(toJson(data)).toJson())
#define fromJsonString(data, json) fromJson(data, (QJsonDocument::fromJson(jsonUtf8)));

QJsonObject toJson(const struct ScratchParameter& data);
int fromJson(struct ScratchParameter& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchArea& data);
int fromJson(struct ScratchArea& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResult& data);
int fromJson(struct ScratchResult& data, const QJsonObject& json);

QJsonObject toJson(const struct ScratchResultFrame& data);
int fromJson(struct ScratchResultFrame& data, const QJsonObject& json);
